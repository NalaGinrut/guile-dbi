#!/usr/bin/guile -s
!#
(use-modules (dbi dbi))

(define db-path (tmpnam))

(define db-obj
 (dbi-open "sqlite3" db-path)
)

(display "test 1 . create table")
(newline)

(dbi-query db-obj "create table testtable(id int, name varchar(15))")

(display db-obj)
(newline)

(newline)
(display "test 2 . insert")
(newline)

(dbi-query db-obj "insert into testtable ('id', 'name') values('33', 'testname1')")
(dbi-query db-obj "insert into testtable ('id', 'name') values('34', 'testname1')")
(dbi-query db-obj "insert into testtable ('id', 'name') values('44', 'testname2')")

(display db-obj)
(newline)

(newline)
(display "test 3 . select")
(newline)

;existent row
(dbi-query db-obj "select * from testtable where name='testname1'")

(display db-obj)
(newline)

(write (dbi-get_row db-obj))
(newline)

;not existent row
(dbi-query db-obj "select * from testtable where name='testname'")

(display db-obj)
(newline)

(write (dbi-get_row db-obj))
(newline)

(newline)
(display "test 4 . count")
(newline)

(dbi-query db-obj "select count(id) from testtable")

(display db-obj)
(newline)

(write (dbi-get_row db-obj))
(newline)

(dbi-close db-obj)
(delete-file db-path)
