"""Parking model. All commands run on the GUI thread, with an injectable clock."""
from dataclasses import dataclass
import json
from pathlib import Path
import re
import time
from structures import Graph, IndexedMinHeap, ReservationStack, CircularBuffer


@dataclass
class Booking:
    code: str
    plate: str
    slot: str
    deadline: float
    status: str = 'RESERVED'


class ParkingSystem:
    def __init__(self, clock=None, graph_file=None, ttl=300):
        self.clock = clock or time.monotonic
        self.offset = 0.0
        self.ttl = ttl
        if ttl <= 0:
            raise ValueError('TTL must be positive')
        source = Path(graph_file) if graph_file else Path(__file__).parent / 'data' / 'parking_graph.json'
        config = json.loads(source.read_text(encoding='utf-8'))
        self.graph = Graph(config['edges'])
        self.distance, self.previous = self.graph.shortest_paths(config['entrance'])
        self.entrance = config['entrance']
        self.slots = {f'P{i}': None for i in range(1, 11)}
        if any(slot not in self.distance for slot in self.slots):
            raise ValueError('Graph must include all ten slots')
        self.by_code = {}  # Python dict is the hash table implementation.
        self.by_plate = {}
        self.expirations = IndexedMinHeap()
        self.undo_stack = ReservationStack()
        self.logs = CircularBuffer(30)
        self.sequence = 0

    def now(self):
        return self.clock() + self.offset

    @staticmethod
    def normalize(plate):
        return ''.join(plate.upper().split())

    def _log(self, action, booking):
        self.logs.append({'time': round(self.now(), 1), 'action': action,
                          'code': booking.code, 'plate': booking.plate, 'slot': booking.slot})

    def _release(self, booking, action):
        if booking.status == 'RESERVED':
            self.expirations.remove(booking.code)
            self.undo_stack.remove(booking.code)
        self.slots[booking.slot] = None
        del self.by_code[booking.code]
        del self.by_plate[booking.plate]
        booking.status = action
        self._log(action, booking)

    def expire(self):
        expired = []
        while self.expirations.peek() and self.expirations.peek()[0] <= self.now():
            booking = self.by_code[self.expirations.peek()[2]]
            self._release(booking, 'EXPIRED')
            expired.append(booking)
        return expired

    def reserve(self, plate):
        self.expire()
        plate = self.normalize(plate)
        if not plate or len(plate) > 20:
            raise ValueError('กรุณากรอกทะเบียน 1-20 ตัวอักษร')
        if re.fullmatch(r'R\d+', plate):
            raise ValueError('ทะเบียนต้องไม่ใช้รูปแบบเดียวกับรหัสจอง R ตามด้วยตัวเลข')
        if plate in self.by_plate:
            raise ValueError('ทะเบียนนี้มีสิทธิ์จองหรือจอดอยู่แล้ว')
        free = [slot for slot, booking in self.slots.items()
                if booking is None and self.distance[slot] != float('inf')]
        if not free:
            raise ValueError('ไม่มีช่องว่างที่เข้าถึงได้')
        slot = min(free, key=lambda s: (self.distance[s], int(s[1:])))
        self.sequence += 1
        code = f'R{self.sequence:04d}'
        booking = Booking(code, plate, slot, self.now() + self.ttl)
        self.slots[slot] = booking
        self.by_code[code] = booking
        self.by_plate[plate] = booking
        self.expirations.push((booking.deadline, self.sequence, code))
        self.undo_stack.push(code)
        self._log('RESERVE', booking)
        return booking

    def lookup(self, credential):
        key = self.normalize(credential)
        booking = self.by_code.get(key) or self.by_plate.get(key)
        if not booking:
            raise ValueError('ไม่พบรหัสจองหรือทะเบียนที่มีสิทธิ์')
        return booking

    def enter(self, credential):
        self.expire()  # At the deadline exactly, the booking is already expired.
        booking = self.lookup(credential)
        if booking.status != 'RESERVED':
            raise ValueError('รถคันนี้เข้าจอดแล้ว')
        self.expirations.remove(booking.code)
        self.undo_stack.remove(booking.code)
        booking.status = 'OCCUPIED'
        self._log('ENTER', booking)
        return booking

    def leave(self, credential):
        self.expire()
        booking = self.lookup(credential)
        if booking.status != 'OCCUPIED':
            raise ValueError('รถยังไม่ได้เข้าจอด กรุณาใช้ยกเลิกการจอง')
        self._release(booking, 'EXIT')
        return booking

    def undo(self):
        self.expire()
        if not self.undo_stack.top:
            raise ValueError('ไม่มีรายการจองที่รอเข้าให้ยกเลิก')
        booking = self.by_code[self.undo_stack.top.key]
        self._release(booking, 'CANCEL')
        return booking

    def advance(self, seconds):
        if seconds < 0:
            raise ValueError('Cannot reverse time')
        self.offset += seconds
        return self.expire()

    def route(self, slot):
        route = [slot]
        while route[-1] != self.entrance:
            route.append(self.previous[route[-1]])
        return list(reversed(route))

    def load_demo(self):
        if self.by_code:
            raise ValueError('โหลดตัวอย่างได้เมื่อลานว่างเท่านั้น')
        a = self.reserve('DEMO-001')
        self.enter(a.code)
        b = self.reserve('DEMO-002')
        self.enter(b.code)

    def check_invariants(self):
        active = [booking for booking in self.slots.values() if booking]
        assert len(active) == len(self.by_code) == len(self.by_plate)
        reserved = {b.code for b in active if b.status == 'RESERVED'}
        assert reserved == set(self.expirations.positions) == set(self.undo_stack.nodes)
        for b in active:
            assert self.by_code[b.code] is b and self.by_plate[b.plate] is b
        assert len(self.logs.values()) <= 30
        return True
