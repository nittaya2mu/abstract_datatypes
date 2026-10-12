
=== Doubly Linked List — กรณีปกติ ===
  ✓ append เพิ่มขนาดและตั้ง head/tail ถูกต้อง
  ✓ find คืนค่า node ที่ถูกต้องด้วย id
  ✓ toArray(true) เรียงจาก head -> tail, toArray(false) เรียงจาก tail -> head

=== Doubly Linked List — กรณีขอบ (edge cases) ===
  ✓ list ว่าง: head/tail เป็น null, size เป็น 0, toArray คืน []
  ✓ find id ที่ไม่มีอยู่จริง คืนค่า null
  ✓ remove id ที่ไม่มีอยู่จริง คืนค่า false และไม่กระทบ list
  ✓ remove node เดียวใน list: head และ tail ต้องกลายเป็น null
  ✓ remove head: head ตัวใหม่ต้องถูกต้อง และ prev ของ node ใหม่เป็น null
  ✓ remove tail: tail ตัวใหม่ต้องถูกต้อง และ next ของ node ใหม่เป็น null
  ✓ remove node ตรงกลาง: prev/next ของเพื่อนบ้านต้องเชื่อมกันใหม่
  ✓ update แก้ไขข้อมูลโดยตำแหน่งใน list ไม่เปลี่ยน

=== AVL Tree — กรณีปกติและกรณีขอบ ===
  ✓ insert แล้วต้นไม้สมดุล (height <= 1.45 * log2(n+2))
  ✓ rangeSearch บนต้นไม้ว่าง คืนค่า [] โดยไม่ error
  ✓ rangeSearch ครอบคลุมทุกค่าต้องได้ทุก id กลับมา
  ✓ insert key ซ้ำ: เก็บหลาย id ไว้ใน node เดียวกัน
  ✓ remove leaf node ไม่กระทบโครงสร้างส่วนอื่น
  ✓ remove node ที่มีลูก 2 ฝั่ง ยังคง rangeSearch ได้ถูกต้องครบ
  ✓ remove AVL node ที่มีลูก 2 ฝั่งและ successor มีหลาย id ต้องไม่ทำให้ id ซ้ำ
  ✓ remove id/key ที่ไม่มีอยู่จริง ไม่ error และไม่ลบอะไรทิ้ง

=== Hash Table — กรณีปกติและกรณีขอบ ===
  ✓ add สะสมยอดเงินในหมวดหมู่เดียวกันถูกต้อง
  ✓ get หมวดหมู่ที่ไม่มีอยู่จริง คืนค่า 0 ไม่ error
  ✓ has ตรวจสอบการมีอยู่ของ key ได้ถูกต้อง
  ✓ remove บนหมวดหมู่ที่ไม่มีอยู่จริง คืนค่า false
  ✓ entries() เรียงจากยอดมากไปน้อยถูกต้อง
  ✓ หลายหมวดหมู่ที่อาจชนกันใน bucket เดียวกันยังแยกยอดกันถูกต้อง (separate chaining)
  ✓ remove Hash Table เมื่อยอดหมด ต้องลบ entry ออกจาก table

=== Sorting Algorithms — กรณีปกติและกรณีขอบ ===
  ✓ quickSort และ mergeSort เรียง array ว่างได้โดยไม่ error
  ✓ quickSort และ mergeSort เรียง array ที่มีตัวเดียวได้ถูกต้อง
  ✓ เรียง array ที่เรียงอยู่แล้ว (already sorted)
  ✓ เรียง array ที่เรียงกลับด้าน (reverse sorted)
  ✓ เรียงแบบ desc ได้ถูกต้อง
  ✓ mergeSort เป็น stable sort: รายการ key ซ้ำกันต้องคงลำดับเดิม
  ✓ quickSort และ mergeSort ให้ผลลัพธ์ตรงกันบนข้อมูลสุ่มขนาดใหญ่ (cross-check ความถูกต้อง)

=== Budget Optimization (Greedy + Knapsack) — กรณีปกติและกรณีขอบ ===
  ✓ งบไม่เกิน: คืนค่า overBudget=false และไม่มีรายการให้ตัด
  ✓ งบ = 0 และมีรายจ่าย: ต้องเกินงบทันทีเท่ากับยอดรายจ่ายทั้งหมด
  ✓ ไม่มีรายการที่ necessity >= 3 ให้ตัด: cutList ว่าง และ stillOver เท่ากับยอดที่เกิน
  ✓ Greedy: necessity เท่ากันต้องเรียงตามจำนวนเงินมาก -> น้อยก่อน
  ✓ Greedy: ตัดครบพอดีจนถึงงบ ต้อง achieved=true และ stillOver=0
  ✓ Knapsack บน overBudgetAmount <= 0 คืนค่า cutList ว่างทันที
  ✓ Knapsack: totalSaved ต้อง >= overBudgetAmount เมื่อทำได้ (ตัดได้พอ)
  ✓ Knapsack ให้ผลประหยัดดีกว่าหรือเท่ากับ Greedy เสมอ (ความเหมาะสมที่สุดของ DP)

=== Wishlist Savings Planner — กรณีปกติและกรณีขอบ ===
  ✓ Equal plan: จำนวนเงินต่อวัน x จำนวนวัน ต้อง >= ราคาสินค้า
  ✓ Equal plan: days เป็น 0 หรือติดลบต้องถูกบังคับเป็นอย่างน้อย 1 วัน (กันหารด้วยศูนย์)
  ✓ Multiplier plan: ผลรวมสะสมวันสุดท้ายต้อง >= ราคาสินค้า
  ✓ Multiplier plan: ราคาต่ำกว่าเงินวันแรก ต้องใช้แค่ 1 วัน
  ✓ Custom plan: จำนวนวันคำนวณถูกต้อง (ceil ของ remaining/dailyAmount)
  ✓ suggestedAmountForNextDeposit ไม่แนะนำเกินยอดที่เหลือ (ป้องกันฝากเกินเป้าหมาย)

=== Regression Tests — บั๊กที่เคยหลุดจากชุดทดสอบเดิม ===
  ✓ Merge Sort รองรับ key แบบ string/category
  ✓ Quick Sort ไม่ stack overflow กับข้อมูลเรียง 20,000 รายการ
  ✓ Knapsack รองรับจำนวนเงินทศนิยม
  ✓ Timestamp รองรับ HH:MM และ HH:MM:SS
  ✓ AVL rangeSearch ช่วงกลับด้านคืน []

==================================================
ผลรวม: ผ่าน 52 / ล้มเหลว 0 / ทั้งหมด 52
==================================================

หมายเหตุ: เพิ่ม Regression Tests สำหรับ string/category sorting, Quick Sort ข้อมูลเรียงขนาดใหญ่, decimal Knapsack, timestamp HH:MM:SS และช่วง AVL กลับด้าน
