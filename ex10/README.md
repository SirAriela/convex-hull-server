# שלב 10 - Producer Consumer - עשר נקודות

## תיאור הבעיה

השרת מיישם דפוס producer-consumer באמצעות POSIX condition variables:

### תרחיש ראשון:
- כאשר שטח ה-CH (Convex Hull) מגיע ל-100 יחידות או יותר
- thread נוסף מתעורר ומדפיס ל-stdout בצד השרת

### תרחיש שני:
- כאשר שטח ה-CH שכבר היה 100 יחידות או יותר, יורד מתחת ל-100
- thread נוסף מתעורר באמצעות POSIX condition variables

## מבנה הפתרון

### Threads:
1. **Area Monitor Thread** - מחכה לשטח CH >= 100
2. **Printer Thread** - מחכה לשטח CH לרדת מתחת ל-100
3. **Client Handler Threads** - מטפלים בלקוחות
4. **Main Thread** - מטפל בפקודות console

### Synchronization:
- `pthread_mutex_t area_mutex` - הגנה על משתני השטח
- `pthread_cond_t area_condition` - condition variable להעברת הודעות
- `std::mutex graph_mutex` - הגנה על הגרף המשותף

### משתנים גלובליים:
- `ch_area_above_100` - האם השטח >= 100
- `ch_area_below_100` - האם השטח ירד מתחת ל-100

## הוראות הרצה

```bash
# בנייה
make all

# הרצת השרת
./convex_hull_server <port>

# הרצת לקוח
./convex_hull_client <server_ip> <port>
```

## פקודות זמינות

- `Newgraph` - יצירת גרף חדש
- `Newpoint x y` - הוספת נקודה
- `Removepoint x y` - הסרת נקודה  
- `CH` - חישוב Convex Hull
- `status` - הצגת מצב הגרף (בשרת)
- `quit` - יציאה (בשרת)

## דוגמה לשימוש

1. הפעל את השרת: `./convex_hull_server 9034`
2. הפעל לקוח: `./convex_hull_client 127.0.0.1 9034`
3. הוסף נקודות עד שהשטח מגיע ל-100+ יחידות
4. הסר נקודות עד שהשטח יורד מתחת ל-100
5. צפה בהודעות השרת על שינויי השטח 