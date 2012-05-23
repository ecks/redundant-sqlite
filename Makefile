ALL:
	gcc -ggdb -o red_db lib/Class.c lib/String.c lib/MsgStamp.c lib/Integer.c lib/RetValData.c thread.c lib/list.c lib/stream.c red_db.c -l sqlite3 -l pthread -std=gnu99
