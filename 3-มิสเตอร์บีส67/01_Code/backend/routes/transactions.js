const express = require('express');
const router = express.Router();
const c = require('../controllers/transactionController');

// สำคัญ: ต้องประกาศ /search ก่อน /:id ไม่เช่นนั้น Express จะตีความ "search" เป็นค่า :id
router.get('/transactions/search', c.search);
router.get('/transactions', c.list);
router.post('/transactions', c.create);
router.put('/transactions/:id', c.update);
router.delete('/transactions/:id', c.remove);

router.get('/dashboard', c.dashboard);

router.get('/budget', c.getBudget);
router.post('/budget', c.setBudget);

router.post('/optimization', c.optimize);

module.exports = router;
