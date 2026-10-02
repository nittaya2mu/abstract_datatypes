# ============================================================
#  Makefile  --  พิมพ์ make แล้วได้ไฟล์ ferry
#  make        คอมไพล์
#  make run    คอมไพล์แล้วรันเลย
#  make debug  คอมไพล์พร้อมตัวตรวจ memory leak (ใช้ตอนไล่บั๊ก)
#  make clean  ลบไฟล์ที่คอมไพล์ออก
# ============================================================
CC      = gcc
# -MMD -MP สร้างไฟล์ .d บันทึกว่าแต่ละ .c พึ่งพา .h ตัวไหนบ้าง
# ถ้าไม่มีบรรทัดนี้ แก้ไฟล์ .h แล้ว make จะไม่คอมไพล์ .c ที่ใช้ .h นั้นใหม่
# ทำให้ .o เก่ากับ .o ใหม่มองโครงสร้าง struct คนละขนาด -> โปรแกรม crash แบบหาสาเหตุยาก
CFLAGS  = -std=c99 -Wall -Wextra -g -MMD -MP
TARGET  = ferry
OBJS    = main.o booking.o graph.o pqueue.o queue.o stack.o hashtable.o avltree.o data.o

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

# ดึงไฟล์ .d ที่ compiler สร้างไว้เข้ามา (เครื่องหมาย - คือไม่ต้องเตือนถ้ายังไม่มีไฟล์)
-include $(OBJS:.o=.d)

run: $(TARGET)
		./$(TARGET)

debug: CFLAGS += -fsanitize=address -fsanitize=undefined
debug: clean $(TARGET)
		./$(TARGET)

clean:
		rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET)

.PHONY: all run debug clean
