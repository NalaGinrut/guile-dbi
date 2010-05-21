/* guile-dbd-sqlite3.c - main source file
 * Copyright (C) 2009 Julian Kalbhenn <jkal@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>. */
#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <sqlite3.h>
#include <errno.h>

#define SET_DBH_STATUS(_K, _V) \
 (* dbh).status = scm_cons(\
  scm_from_int(_K),\
  scm_from_locale_string(_V)\
 );

#define b0 void
#define b8 char
#define b32 int

b0 __sqlite3_make_g_db_handle(gdbi_db_handle_t *);
b0 __sqlite3_close_g_db_handle(gdbi_db_handle_t *);
b0 __sqlite3_query_g_db_handle(gdbi_db_handle_t *, b8 *);
SCM __sqlite3_getrow_g_db_handle(gdbi_db_handle_t *);

typedef struct{
 sqlite3 * sqlite3_obj;
 sqlite3_stmt * stmt;
} gdbi_sqlite3_ds_t;


b0 __sqlite3_make_g_db_handle(
 gdbi_db_handle_t * dbh
){
 (* dbh).closed = SCM_BOOL_T;

 //check presence of connection string
 if(scm_equal_p(scm_string_p((* dbh).constr), SCM_BOOL_F) == SCM_BOOL_T){
  SET_DBH_STATUS(1, "missing connection string");

  return;
 }

 b8 * db_name = scm_to_locale_string((* dbh).constr);

 gdbi_sqlite3_ds_t * db_info = (gdbi_sqlite3_ds_t *) malloc(sizeof(gdbi_sqlite3_ds_t));

 //if error
 if(db_info == 0){
  SET_DBH_STATUS(1, "out of memory");

  return;
 }

 b8 tres = sqlite3_open(
  db_name,
  &((* db_info).sqlite3_obj)
 );

 //if error
 if(tres != SQLITE_OK){
  SET_DBH_STATUS(1, sqlite3_errmsg((* db_info).sqlite3_obj));
  (* db_info).sqlite3_obj = 0;
  free(db_info);
  (* dbh).db_info = 0;

  return;
 }

 (* dbh).db_info = db_info;

 SET_DBH_STATUS(0, "db connected");
 (* dbh).closed = SCM_BOOL_F;

 return;
}


b0 __sqlite3_close_g_db_handle(gdbi_db_handle_t * dbh){
 //check presence of db object
 if((* dbh).db_info == 0){
  if(!(* dbh).in_free){
   SET_DBH_STATUS(1, "dbd info not found");
  }

  return;
 }

 gdbi_sqlite3_ds_t * db_info = (* dbh).db_info;

 if((* db_info).sqlite3_obj == 0){
  if(!(* dbh).in_free){
   SET_DBH_STATUS(1, "dbi connection already closed");
   free((* dbh).db_info);
   (* dbh).db_info = 0;

   return;
  }
 }

 sqlite3_close(
  (* db_info).sqlite3_obj
 );

 free((* dbh).db_info);
 (* dbh).db_info = 0;

 (* dbh).closed = SCM_BOOL_T;

 if(!(* dbh).in_free){ SET_DBH_STATUS(0, "dbi closed"); }

 return;
}


b0 __sqlite3_query_g_db_handle(gdbi_db_handle_t * dbh, b8 * query_str){
 if((* dbh).db_info == 0){
  SET_DBH_STATUS(1, "invalid dbi connection");

  return;
 }

 gdbi_sqlite3_ds_t * db_info = (* dbh).db_info;
 (* db_info).stmt = 0;

 //clear previous result
 if((* db_info).stmt != 0){
  sqlite3_finalize((* db_info).stmt);
  (* db_info).stmt = 0;
 }

 sqlite3_stmt * stmt;
 b8 tres = sqlite3_prepare_v2((* db_info).sqlite3_obj, query_str, -1, &stmt, 0);
 if(tres != SQLITE_OK){
  SET_DBH_STATUS(1,  sqlite3_errmsg((* db_info).sqlite3_obj));

  return;
 }

 //test if sqlite3_step runs successful
 tres = sqlite3_step(stmt);
 if((tres != SQLITE_ROW) && (tres != SQLITE_DONE) && (tres != SQLITE_OK)){
  SET_DBH_STATUS(1,  sqlite3_errmsg((* db_info).sqlite3_obj));

  return;
 }
 sqlite3_reset(stmt);

 (* db_info).stmt = stmt;
 SET_DBH_STATUS(0, "query ok");
 return;
}


SCM __sqlite3_getrow_g_db_handle(gdbi_db_handle_t * dbh){
 gdbi_sqlite3_ds_t * db_info = (* dbh).db_info;

 if(db_info == 0){
  SET_DBH_STATUS(1, "invalid dbi connection");

  return(SCM_BOOL_F);
 }

 #define STMT (* db_info).stmt

 if(STMT == 0){
  SET_DBH_STATUS(1, "missing query result");

  return(SCM_BOOL_F);
 }

 SCM res_row = SCM_EOL;
 SCM cur_val;

 b8 tres = sqlite3_step(STMT);

 if(tres == SQLITE_ROW){
  //row to scheme list
  b32 col_count = sqlite3_column_count(STMT);
  b32 cur_col_idx = 0;
  b8 col_type;

  while(cur_col_idx < col_count){
   col_type = sqlite3_column_type(STMT, cur_col_idx);
   if(col_type == SQLITE_INTEGER){
    cur_val = scm_from_long(sqlite3_column_int(STMT, cur_col_idx));
   }else if(col_type == SQLITE_FLOAT){
    cur_val = scm_from_double(sqlite3_column_double(STMT, cur_col_idx));
   }else if(col_type == SQLITE_TEXT){
    cur_val = scm_from_locale_string(sqlite3_column_text(STMT, cur_col_idx));
   }else if(col_type == SQLITE_BLOB){
    cur_val = scm_from_locale_string(sqlite3_column_blob(STMT, cur_col_idx));
   }else if(col_type == SQLITE_NULL){
    cur_val = SCM_BOOL_F;
   }else{
    SET_DBH_STATUS(1, "unknown field type");

    return(SCM_EOL);
   }
   res_row = scm_append( scm_list_2(
     res_row,
     scm_list_1( scm_cons(
       scm_from_locale_string( sqlite3_column_name(STMT, cur_col_idx)),
       cur_val
      )
     )
    )
   );

   cur_col_idx += 1;
  }
 }else if(tres == SQLITE_DONE){
  SET_DBH_STATUS(1, "no more rows to get");

  return(SCM_BOOL_F);
 }else{
  SET_DBH_STATUS(1, sqlite3_errmsg((* db_info).sqlite3_obj)); //probably "unknown error"
  sqlite3_finalize(STMT);

  return(SCM_BOOL_F);
 }

 SET_DBH_STATUS(0, "row fetched");

 #undef STMT





 return(res_row);
}
