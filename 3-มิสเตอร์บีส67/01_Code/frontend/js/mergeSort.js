/**
 * Merge Sort (Divide and Conquer)
 * Worst / Average O(n log n), stable
 */
function mergeSort(items, keyFn, order = 'asc') {
  let comparisons = 0;
  let merges = 0;
  const start = performance.now();

  const cmp = (a, b) => {
    comparisons++;
    const va = keyFn(a), vb = keyFn(b);
    let c;
    if (typeof va === 'number' && typeof vb === 'number' && Number.isFinite(va) && Number.isFinite(vb)) {
      c = va < vb ? -1 : va > vb ? 1 : 0;
    } else {
      c = String(va ?? '').localeCompare(String(vb ?? ''), 'th', { numeric: true, sensitivity: 'base' });
    }
    return order === 'asc' ? c : -c;
  };

  function merge(left, right) {
    merges++;
    const out = [];
    let i = 0, j = 0;
    while (i < left.length && j < right.length) {
      if (cmp(left[i], right[j]) <= 0) out.push(left[i++]);
      else out.push(right[j++]);
    }
    while (i < left.length) out.push(left[i++]);
    while (j < right.length) out.push(right[j++]);
    return out;
  }

  function sort(a) {
    if (a.length <= 1) return a;
    const mid = Math.floor(a.length / 2);
    return merge(sort(a.slice(0, mid)), sort(a.slice(mid)));
  }

  const result = sort([...items]);
  const end = performance.now();
  return { result, algorithm: 'Merge Sort', comparisons, merges, timeMs: end - start, n: result.length };
}

if (typeof module !== 'undefined') module.exports = { mergeSort };
