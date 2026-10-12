/**
 * Quick Sort
 * Iterative 3-way partition + median-of-three pivot.
 * Average O(n log n), worst O(n^2), but no recursive call-stack overflow.
 */
function quickSort(items, keyFn, order = 'asc') {
  const arr = [...items];
  let comparisons = 0;
  let swaps = 0;
  const start = performance.now();

  const cmpValue = (va, vb) => {
    let c;
    if (typeof va === 'number' && typeof vb === 'number' && Number.isFinite(va) && Number.isFinite(vb)) {
      c = va < vb ? -1 : va > vb ? 1 : 0;
    } else {
      c = String(va ?? '').localeCompare(String(vb ?? ''), 'th', { numeric: true, sensitivity: 'base' });
    }
    return order === 'asc' ? c : -c;
  };
  const cmp = (a, b) => { comparisons++; return cmpValue(keyFn(a), keyFn(b)); };
  const swap = (a, i, j) => {
    if (i === j) return;
    const t = a[i]; a[i] = a[j]; a[j] = t; swaps++;
  };
  const medianIndex = (a, i, j, k) => {
    if (cmp(a[i], a[j]) < 0) {
      if (cmp(a[j], a[k]) < 0) return j;
      return cmp(a[i], a[k]) < 0 ? k : i;
    }
    if (cmp(a[i], a[k]) < 0) return i;
    return cmp(a[j], a[k]) < 0 ? k : j;
  };

  const stack = arr.length > 1 ? [[0, arr.length - 1]] : [];
  while (stack.length) {
    const [lo, hi] = stack.pop();
    if (lo >= hi) continue;

    const mid = lo + Math.floor((hi - lo) / 2);
    const pIdx = medianIndex(arr, lo, mid, hi);
    const pivot = arr[pIdx];

    // Dutch-national-flag partition: < pivot | == pivot | > pivot
    let lt = lo, i = lo, gt = hi;
    while (i <= gt) {
      const c = cmp(arr[i], pivot);
      if (c < 0) { swap(arr, lt, i); lt++; i++; }
      else if (c > 0) { swap(arr, i, gt); gt--; }
      else i++;
    }

    // Process smaller partition first to keep explicit stack small.
    const left = [lo, lt - 1];
    const right = [gt + 1, hi];
    if (left[1] - left[0] > right[1] - right[0]) {
      if (left[0] < left[1]) stack.push(left);
      if (right[0] < right[1]) stack.push(right);
    } else {
      if (right[0] < right[1]) stack.push(right);
      if (left[0] < left[1]) stack.push(left);
    }
  }

  const end = performance.now();
  return { result: arr, algorithm: 'Quick Sort', comparisons, swaps, timeMs: end - start, n: arr.length };
}

if (typeof module !== 'undefined') module.exports = { quickSort };
