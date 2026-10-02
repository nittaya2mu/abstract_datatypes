import random
import unittest
from parking import ParkingSystem
from structures import Graph, IndexedMinHeap, ReservationStack, CircularBuffer


class ParkingTests(unittest.TestCase):
    def setUp(self):
        self.time = 0
        self.s = ParkingSystem(clock=lambda: self.time)

    def test_nearest_route(self):
        self.s.load_demo()
        b = self.s.reserve('TEST-003')
        self.assertEqual(b.slot, 'P3')
        self.assertEqual(self.s.distance['P3'], 22)
        self.assertEqual(self.s.route('P3'), ['Entrance', 'J1', 'J2', 'P3'])

    def test_weighted_shortcut(self):
        g = Graph([('A', 'B', 10), ('A', 'C', 1), ('C', 'B', 2)])
        self.assertEqual(g.shortest_paths('A')[0]['B'], 3)

    def test_full(self):
        for i in range(10):
            self.s.reserve(str(i))
        with self.assertRaises(ValueError):
            self.s.reserve('11')

    def test_duplicate_and_validation(self):
        self.s.reserve('ab 123')
        for plate in ['AB123', '', 'x'*21, 'R0001']:
            with self.assertRaises(ValueError):
                self.s.reserve(plate)

    def test_expiry_exact_boundary(self):
        b = self.s.reserve('A')
        self.time = 300
        with self.assertRaises(ValueError):
            self.s.enter(b.code)
        self.assertIsNone(self.s.slots['P1'])

    def test_entry_removes_expiration_and_exit(self):
        b = self.s.reserve('A')
        self.s.enter('a')
        self.s.advance(600)
        self.assertEqual(b.status, 'OCCUPIED')
        self.assertEqual(len(self.s.expirations), 0)
        self.s.leave(b.code)
        self.assertTrue(self.s.check_invariants())

    def test_undo_lifo_skips_entered(self):
        a = self.s.reserve('A')
        b = self.s.reserve('B')
        c = self.s.reserve('C')
        self.s.enter(b.code)
        self.assertEqual(self.s.undo().code, c.code)
        self.assertEqual(self.s.undo().code, a.code)
        with self.assertRaises(ValueError):
            self.s.undo()

    def test_staggered_deadlines(self):
        a = self.s.reserve('A')
        self.time = 60
        b = self.s.reserve('B')
        self.time = 300
        self.assertEqual([x.code for x in self.s.expire()], [a.code])
        self.assertIn(b.code, self.s.by_code)

    def test_invalid_transitions(self):
        a = self.s.reserve('A')
        with self.assertRaises(ValueError):
            self.s.leave(a.code)
        self.s.enter(a.code)
        with self.assertRaises(ValueError):
            self.s.enter(a.code)
        with self.assertRaises(ValueError):
            self.s.enter('missing')

    def test_ring_overwrite(self):
        ring = CircularBuffer(30)
        for i in range(35):
            ring.append(i)
        self.assertEqual(ring.values(), list(range(5,35)))

    def test_heap_arbitrary_delete(self):
        h = IndexedMinHeap()
        rng = random.Random(14)
        expected = [(rng.randrange(100), i, str(i)) for i in range(200)]
        for item in expected:
            h.push(item)
        for item in expected[::3]:
            h.remove(item[2])
        remaining = sorted(item for item in expected if item not in expected[::3])
        self.assertEqual([h.pop() for _ in range(len(h))], remaining)

    def test_stack_unlink(self):
        stack = ReservationStack()
        for key in ['A','B','C']:
            stack.push(key)
        stack.remove('B')
        self.assertEqual(stack.pop(), 'C')
        self.assertEqual(stack.pop(), 'A')

    def test_long_rotation_is_bounded(self):
        for i in range(1000):
            b = self.s.reserve(str(i))
            if i % 3 == 0:
                self.s.enter(b.code)
                self.s.leave(b.code)
            elif i % 3 == 1:
                self.s.undo()
            else:
                self.s.advance(300)
            self.s.check_invariants()
        self.assertEqual(len(self.s.by_code), 0)
        self.assertEqual(len(self.s.undo_stack.nodes), 0)
        self.assertEqual(self.s.logs.count, 30)

    def test_mixed_random_operations(self):
        rng = random.Random(14)
        for i in range(1500):
            operation = rng.randrange(5)
            try:
                if operation == 0:
                    self.s.reserve(str(rng.randrange(15)))
                elif operation == 1:
                    self.s.enter(str(rng.randrange(15)))
                elif operation == 2:
                    self.s.leave(str(rng.randrange(15)))
                elif operation == 3:
                    self.s.undo()
                else:
                    self.s.advance(rng.randrange(90))
            except ValueError:
                pass
            self.s.check_invariants()


if __name__ == '__main__':
    unittest.main()
