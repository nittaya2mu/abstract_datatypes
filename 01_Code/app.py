"""Thai desktop interface. Run with Python 3.10+ or the bundled Windows EXE."""
import argparse
import csv
from pathlib import Path
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from parking import ParkingSystem

BG = '#f0f4f7'
INK = '#132c3c'
COLORS = {'FREE': '#dff4e9', 'RESERVED': '#fff0c6', 'OCCUPIED': '#dbe9fb'}
THAI = {'FREE': 'ว่าง', 'RESERVED': 'จองแล้ว', 'OCCUPIED': 'เข้าจอดแล้ว'}
EVENT = {'RESERVE': 'จอง', 'ENTER': 'เข้า', 'EXIT': 'ออก', 'CANCEL': 'ยกเลิก', 'EXPIRED': 'หมดอายุ'}


class ParkingApp:
    def __init__(self, root, demo=False):
        self.root = root
        self.system = ParkingSystem()
        self.started = self.system.now()
        root.title('SmartParking10 - ADT Group 14')
        root.geometry('1180x880')
        root.minsize(1080, 810)
        root.configure(bg=BG)
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('.', font=('Tahoma', 11))
        style.configure('TFrame', background=BG)
        style.configure('TLabel', background=BG, foreground=INK)
        style.configure('TButton', padding=(12, 8))
        style.configure('Treeview', rowheight=28, font=('Tahoma', 10))
        style.configure('Treeview.Heading', font=('Tahoma', 10, 'bold'))
        outer = ttk.Frame(root, padding=24)
        outer.pack(fill='both', expand=True)
        ttk.Label(outer, text='SmartParking10', font=('Tahoma', 26, 'bold')).pack(anchor='w')
        ttk.Label(outer, text='จองที่จอดใกล้ทางเข้าอัตโนมัติ  /  ลานจอด 10 ช่อง  /  กลุ่ม 14', font=('Tahoma', 12)).pack(anchor='w', pady=(2, 12))
        self.stats = ttk.Label(outer, font=('Tahoma', 13, 'bold'))
        self.stats.pack(anchor='w', pady=(0, 10))
        grid = ttk.Frame(outer)
        grid.pack(fill='x')
        self.tiles = {}
        for i in range(10):
            tile = tk.Label(grid, font=('Tahoma', 11), justify='left', padx=14, pady=12, anchor='nw', height=4)
            tile.grid(row=i // 5, column=i % 5, sticky='nsew', padx=(0, 8), pady=(0, 8))
            grid.columnconfigure(i % 5, weight=1)
            self.tiles[f'P{i+1}'] = tile
        ttk.Label(outer, text='สถานะ: สีเขียว = ว่าง   สีเหลือง = จองแล้ว   สีฟ้า = เข้าจอดแล้ว', font=('Tahoma', 10)).pack(anchor='w', pady=(0, 10))
        controls = ttk.Frame(outer)
        controls.pack(fill='x')
        ttk.Label(controls, text='ทะเบียนรถ').grid(row=0, column=0, sticky='w')
        self.plate = ttk.Entry(controls, width=22)
        self.plate.grid(row=0, column=1, padx=8, pady=4)
        ttk.Button(controls, text='จองช่องที่ใกล้ที่สุด', command=self.reserve).grid(row=0, column=2, padx=4)
        ttk.Button(controls, text='ยกเลิกการจองล่าสุด', command=self.undo).grid(row=0, column=3, padx=4)
        ttk.Label(controls, text='รหัสจอง / ทะเบียน').grid(row=1, column=0, sticky='w')
        self.credential = ttk.Entry(controls, width=22)
        self.credential.grid(row=1, column=1, padx=8, pady=4)
        ttk.Button(controls, text='ยืนยันเข้า / เปิดไม้กั้น', command=self.enter).grid(row=1, column=2, padx=4)
        ttk.Button(controls, text='บันทึกออกจากลาน', command=self.leave).grid(row=1, column=3, padx=4)
        self.message = tk.Label(outer, text='พร้อมใช้งาน กรุณากรอกทะเบียนเพื่อจอง', bg='#ffffff', fg=INK,
                                font=('Tahoma', 11), anchor='w', justify='left', padx=12, pady=10, wraplength=1080)
        self.message.pack(fill='x', pady=(12, 8))
        demo_bar = ttk.Frame(outer)
        demo_bar.pack(fill='x')
        ttk.Button(demo_bar, text='โหลดตัวอย่าง P1, P2', command=self.demo).pack(side='left')
        ttk.Button(demo_bar, text='เร่งเวลา +60 วินาที', command=lambda: self.advance(60)).pack(side='left', padx=6)
        ttk.Button(demo_bar, text='เร่งเวลา +5 นาที', command=lambda: self.advance(300)).pack(side='left')
        ttk.Button(demo_bar, text='ส่งออกประวัติ CSV', command=self.export).pack(side='right')
        self.next_expiry = ttk.Label(outer, font=('Tahoma', 10))
        self.next_expiry.pack(anchor='w', pady=10)
        ttk.Label(outer, text='ประวัติล่าสุด 30 รายการ', font=('Tahoma', 13, 'bold')).pack(anchor='w')
        log_frame = ttk.Frame(outer)
        log_frame.pack(fill='both', expand=True, pady=(8, 8))
        columns = ('time', 'action', 'code', 'plate', 'slot')
        self.tree = ttk.Treeview(log_frame, columns=columns, show='headings', height=5)
        for col, title, width in zip(columns, ['เวลาสัมพัทธ์ (วินาที)', 'เหตุการณ์', 'รหัสจอง', 'ทะเบียน', 'ช่อง'], [210, 150, 150, 300, 100]):
            self.tree.heading(col, text=title)
            self.tree.column(col, width=width, anchor='center')
        scrollbar = ttk.Scrollbar(log_frame, orient='vertical', command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side='left', fill='both', expand=True)
        scrollbar.pack(side='right', fill='y')
        ttk.Label(outer, text='โปรแกรมจำลองสำหรับผู้ดูแล 1 คน  •  สิทธิ์จอง 5 นาที  •  ไม้กั้นจำลอง  •  ปิดโปรแกรมแล้วข้อมูลจะเริ่มใหม่', font=('Tahoma', 9)).pack(anchor='w')
        if demo:
            self.system.load_demo()
        self.refresh()
        self.tick()

    def run_action(self, function, success):
        try:
            result = function()
            self.message.configure(text=success(result), fg=INK)
        except ValueError as error:
            self.message.configure(text=str(error), fg='#a62a30')
        self.refresh()

    def reserve(self):
        def success(b):
            self.credential.delete(0, 'end')
            self.credential.insert(0, b.code)
            route = ' > '.join(self.system.route(b.slot))
            return f'จองสำเร็จ {b.code}  ทะเบียน {b.plate}  ช่อง {b.slot}  ระยะ {self.system.distance[b.slot]} เมตร\nเส้นทาง {route}  กรุณาเข้าภายใน 5 นาที'
        self.run_action(lambda: self.system.reserve(self.plate.get()), success)

    def enter(self):
        self.run_action(lambda: self.system.enter(self.credential.get()), lambda b: f'ไม้กั้นเปิด (จำลอง)  ยืนยัน {b.code}  ทะเบียน {b.plate}  เข้าจอดช่อง {b.slot}')

    def leave(self):
        self.run_action(lambda: self.system.leave(self.credential.get()), lambda b: f'บันทึกออกสำเร็จ  {b.plate}  คืนช่อง {b.slot} ให้ว่างแล้ว')

    def undo(self):
        self.run_action(self.system.undo, lambda b: f'ยกเลิกการจองล่าสุด {b.code}  คืนช่อง {b.slot} แล้ว')

    def demo(self):
        self.run_action(self.system.load_demo, lambda _: 'โหลดตัวอย่างแล้ว: P1 และ P2 มีรถเข้าจอด  การจองถัดไปจะได้ P3')

    def advance(self, seconds):
        self.run_action(lambda: self.system.advance(seconds), lambda items: f'เร่งเวลา {seconds} วินาที  ยกเลิกรายการหมดอายุ {len(items)} รายการ  รถที่เข้าจอดแล้วยังคงอยู่')

    def export(self):
        target = filedialog.asksaveasfilename(defaultextension='.csv', filetypes=[('CSV', '*.csv')], initialfile='parking_history.csv')
        if target:
            try:
                with open(target, 'w', newline='', encoding='utf-8-sig') as stream:
                    writer = csv.DictWriter(stream, fieldnames=['time', 'action', 'code', 'plate', 'slot'])
                    writer.writeheader()
                    for row in self.system.logs.values():
                        writer.writerow({**row, 'time': round(row['time'] - self.started, 1)})
                self.message.configure(text='ส่งออกประวัติ CSV สำเร็จ', fg=INK)
            except OSError as error:
                messagebox.showerror('ส่งออกไม่สำเร็จ', str(error))

    def tick(self):
        expired = self.system.expire()
        if expired:
            self.message.configure(text='หมดอายุและคืนช่องแล้ว: ' + ', '.join(b.code + ' / ' + b.slot for b in expired), fg='#a62a30')
        self.refresh()
        self.root.after(250, self.tick)

    def refresh(self):
        free = sum(b is None for b in self.system.slots.values())
        reserved = len(self.system.expirations)
        self.stats.configure(text=f'ว่าง {free} ช่อง     จอง {reserved} ช่อง     เข้าจอด {10-free-reserved} ช่อง')
        for slot, booking in self.system.slots.items():
            status = booking.status if booking else 'FREE'
            label = f'{slot}   {int(self.system.distance[slot])} เมตร\n{THAI[status]}'
            if booking:
                label += f'\n{booking.plate}  {booking.code}'
                if status == 'RESERVED':
                    label += f'\nเหลือ {max(0, int(booking.deadline-self.system.now()))} วินาที'
            self.tiles[slot].configure(text=label, bg=COLORS[status], fg=INK)
        earliest = self.system.expirations.peek()
        self.next_expiry.configure(text=f'Min-Heap: รหัส {earliest[2]} จะหมดอายุใน {max(0,int(earliest[0]-self.system.now()))} วินาที' if earliest else 'Min-Heap: ไม่มีรายการจองรอหมดอายุ')
        rows = self.system.logs.values()
        signature = tuple((row['code'], row['action']) for row in rows)
        if signature != getattr(self, '_log_signature', None):
            self._log_signature = signature
            for item in self.tree.get_children():
                self.tree.delete(item)
            for row in reversed(rows):
                self.tree.insert('', 'end', values=(f"{row['time']-self.started:.1f}", EVENT[row['action']], row['code'], row['plate'], row['slot']))


def main():
    parser = argparse.ArgumentParser(description='SmartParking10 desktop simulation')
    parser.add_argument('--demo', action='store_true')
    parser.add_argument('--self-test', action='store_true', help='headless model smoke test, including frozen EXE')
    args = parser.parse_args()
    if args.self_test:
        model = ParkingSystem(clock=lambda: 0)
        model.load_demo()
        booking = model.reserve('SELF-TEST')
        assert booking.slot == 'P3'
        model.advance(300)
        assert model.slots['P3'] is None
        assert model.check_invariants()
        print('SELF-TEST PASS')
        return
    root = tk.Tk()
    ParkingApp(root, args.demo)
    root.mainloop()


if __name__ == '__main__':
    main()
