"""Small, explicit ADT implementations used by SmartParking10."""
import heapq


class Graph:
    def __init__(self, edges):
        self.adj = {}
        for a, b, weight in edges:
            if weight < 0:
                raise ValueError('Dijkstra requires nonnegative weights')
            self.adj.setdefault(a, []).append((b, weight))
            self.adj.setdefault(b, []).append((a, weight))

    def shortest_paths(self, start):
        if start not in self.adj:
            raise ValueError('Unknown entrance')
        distance = {node: float('inf') for node in self.adj}
        previous = {}
        distance[start] = 0
        queue = [(0, start)]
        while queue:
            cost, node = heapq.heappop(queue)
            if cost != distance[node]:
                continue
            for neighbor, weight in self.adj[node]:
                candidate = cost + weight
                if candidate < distance[neighbor]:
                    distance[neighbor] = candidate
                    previous[neighbor] = node
                    heapq.heappush(queue, (candidate, neighbor))
        return distance, previous


class IndexedMinHeap:
    """A min-heap with O(log n) deletion by booking ID; no stale entries."""
    def __init__(self):
        self.items = []
        self.positions = {}

    def __len__(self):
        return len(self.items)

    def peek(self):
        return self.items[0] if self.items else None

    def _swap(self, a, b):
        self.items[a], self.items[b] = self.items[b], self.items[a]
        self.positions[self.items[a][2]] = a
        self.positions[self.items[b][2]] = b

    def _up(self, index):
        while index and self.items[index] < self.items[(index - 1) // 2]:
            parent = (index - 1) // 2
            self._swap(index, parent)
            index = parent

    def _down(self, index):
        while True:
            left = 2 * index + 1
            if left >= len(self.items):
                return
            right = left + 1
            child = right if right < len(self.items) and self.items[right] < self.items[left] else left
            if self.items[index] <= self.items[child]:
                return
            self._swap(index, child)
            index = child

    def push(self, item):
        if item[2] in self.positions:
            raise ValueError('Duplicate heap key')
        self.positions[item[2]] = len(self.items)
        self.items.append(item)
        self._up(len(self.items) - 1)

    def remove(self, key):
        index = self.positions.pop(key)
        result = self.items[index]
        last = self.items.pop()
        if index < len(self.items):
            self.items[index] = last
            self.positions[last[2]] = index
            if index and self.items[index] < self.items[(index - 1) // 2]:
                self._up(index)
            else:
                self._down(index)
        return result

    def pop(self):
        if not self.items:
            raise IndexError('Empty heap')
        return self.remove(self.items[0][2])


class _Node:
    def __init__(self, key, below):
        self.key = key
        self.below = below
        self.above = None


class ReservationStack:
    """LIFO stack plus O(1) unlink for bookings that enter or expire.

    Doubly linked nodes and an ID index avoid accumulation of stale history.
    This is the latest pending reservation of this single-operator desktop.
    """
    def __init__(self):
        self.top = None
        self.nodes = {}

    def push(self, key):
        node = _Node(key, self.top)
        if self.top:
            self.top.above = node
        self.top = node
        self.nodes[key] = node

    def remove(self, key):
        node = self.nodes.pop(key)
        if node.above:
            node.above.below = node.below
        else:
            self.top = node.below
        if node.below:
            node.below.above = node.above
        return key

    def pop(self):
        if not self.top:
            raise IndexError('Empty stack')
        return self.remove(self.top.key)


class CircularBuffer:
    def __init__(self, capacity=30):
        if capacity <= 0:
            raise ValueError('Capacity must be positive')
        self.capacity = capacity
        self.data = [None] * capacity
        self.next = 0
        self.count = 0

    def append(self, value):
        self.data[self.next] = value
        self.next = (self.next + 1) % self.capacity
        self.count = min(self.count + 1, self.capacity)

    def values(self):
        start = (self.next - self.count) % self.capacity
        return [self.data[(start + i) % self.capacity] for i in range(self.count)]
