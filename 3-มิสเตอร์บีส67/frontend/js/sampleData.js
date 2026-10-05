/**
 * sampleData.js
 * ข้อมูลตัวอย่างสำหรับการเปิดใช้งานครั้งแรก (seed data)
 * เพื่อให้สามารถทดสอบ Dashboard / Search / Sort / Budget Optimization ได้ทันที
 */

function generateSampleData() {
  const today = new Date();
  const d = (offsetDays) => {
    const dt = new Date(today);
    dt.setDate(dt.getDate() - offsetDays);
    return dt.toISOString().slice(0, 10);
  };

  let idCounter = 1;
  const tx = (type, amount, category, offsetDays, time, note, necessity) => ({
    id: 'tx' + String(idCounter++).padStart(4, '0'),
    type, amount, category, date: d(offsetDays), time, note, necessity: type === 'expense' ? necessity : null
  });

  return [
    tx('income', 15000, 'เงินเดือน/ค่าขนม', 27, '08:00', 'เงินโอนจากที่บ้านประจำเดือน', null),
    tx('expense', 1800, 'อาหาร', 25, '12:30', 'ค่าอาหารกลางวันทั้งสัปดาห์', 1),
    tx('expense', 1200, 'เดินทาง', 24, '07:45', 'ค่าน้ำมัน/ค่ารถโดยสารไป-กลับมหาวิทยาลัย', 2),
    tx('expense', 1800, 'การเรียน', 22, '10:00', 'ค่าซื้อหนังสือและอุปกรณ์การเรียน', 1),
    tx('expense', 1400, 'บิล/สาธารณูปโภค', 20, '18:00', 'ค่าอินเทอร์เน็ตห้องพัก', 1),
    tx('expense', 2200, 'ช้อปปิ้ง', 18, '20:15', 'ซื้อหูฟังไร้สายเครื่องใหม่', 5),
    tx('expense', 1500, 'บันเทิง', 15, '21:00', 'ซื้อเกมออนไลน์', 5),
    tx('expense', 900, 'อาหาร', 12, '19:30', 'ชานมไข่มุกและของว่าง', 4),
    tx('expense', 1000, 'บันเทิง', 9, '22:00', 'ค่าสมัครสตรีมมิงเพลง/หนัง', 4),
    tx('expense', 650, 'เดินทาง', 6, '13:00', 'ค่าแท็กซี่ไปธุระด่วน', 2),
    tx('expense', 500, 'อาหาร', 3, '08:15', 'อาหารเช้า', 1),
    tx('expense', 1300, 'ช้อปปิ้ง', 1, '17:00', 'เสื้อผ้าใหม่', 5),
  ];
}

const SAMPLE_BUDGET = 10000;

if (typeof module !== 'undefined') module.exports = { generateSampleData, SAMPLE_BUDGET };
