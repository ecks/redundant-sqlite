ALL:
	gcc -ggdb -o red_db Class.c String.c MsgStamp.c Integer.c thread.c list.c stream.c red_db.c -l sqlite3 -l pthread -std=gnu99
