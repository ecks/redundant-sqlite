ALL:
	gcc -ggdb -o red_db Class.h String.h MsgStamp.h Integer.h thread.h list.h red_db.h Class.c String.c MsgStamp.c Integer.c thread.c list.c red_db.c -l sqlite3 -l pthread -std=gnu99
