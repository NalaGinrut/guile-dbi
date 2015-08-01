/* guile-dbd-sqlite3.c - main source file
 * Copyright (C) 2009-2014 (jkal@posteo.eu, https://github.com/jkalbhenn)
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

void __sqlite3_make_g_db_handle(gdbi_db_handle_t *dbh);
void __sqlite3_close_g_db_handle(gdbi_db_handle_t *dbh);
void __sqlite3_query_g_db_handle(gdbi_db_handle_t *dbh, char *query_str);
SCM __sqlite3_getrow_g_db_handle(gdbi_db_handle_t *dbh);
SCM status_cons(int code, const char *message);

typedef struct{
  sqlite3 *sqlite3_obj;
  sqlite3_stmt *stmt;
} gdbi_sqlite3_ds_t;

SCM status_cons(int code, const char *message)
{
  return(scm_cons(scm_from_int(code), scm_from_locale_string(message)));
}

void __sqlite3_make_g_db_handle(gdbi_db_handle_t *dbh)
{
  dbh->closed = SCM_BOOL_T;
  /* check presence of connection string */
  if (scm_equal_p(scm_string_p(dbh->constr), SCM_BOOL_F) == SCM_BOOL_T) {
    dbh->status = status_cons(1, "missing connection string");
    return;
  }
  char *db_name = scm_to_locale_string(dbh->constr);
  gdbi_sqlite3_ds_t *db_info = malloc(sizeof(gdbi_sqlite3_ds_t));
  if (db_info == NULL) {
    dbh->status = status_cons(1, "out of memory");
    return;
  }
  char s = sqlite3_open(db_name, &(db_info->sqlite3_obj));
  free(db_name);
  if (s != SQLITE_OK) {
    dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));
    free(db_info);
    dbh->db_info = NULL;
    return;
  }
  db_info->stmt = NULL;
  dbh->db_info = db_info;
  dbh->status = status_cons(0, "db connected");
  dbh->closed = SCM_BOOL_F;
}

void __sqlite3_close_g_db_handle(gdbi_db_handle_t *dbh)
{
  /* check presence of db object */
  if (dbh->db_info == NULL) {
    if (!dbh->in_free)
      dbh->status = status_cons(1, "dbd info not found");
    return;
  }
  if (!dbh->in_free) {
    gdbi_sqlite3_ds_t *db_info = dbh->db_info;
    sqlite3_finalize(db_info->stmt);
    sqlite3_close_v2(db_info->sqlite3_obj);
    free(dbh->db_info);
    dbh->db_info = NULL;
    dbh->closed = SCM_BOOL_T;
    dbh->status = status_cons(0, "dbi closed");
  }
}

void __sqlite3_query_g_db_handle(gdbi_db_handle_t *dbh, char *query_str)
{
  if (dbh->db_info == NULL) {
    dbh->status = status_cons(1, "invalid dbi connection");
    return;
  }
  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  sqlite3_finalize(db_info->stmt);
  db_info->stmt = NULL;
  sqlite3_stmt *stmt;
  char s = sqlite3_prepare_v2(db_info->sqlite3_obj,
    query_str, -1, &stmt, NULL);
  if (s != SQLITE_OK) {
    dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));
    return;
  }
  /* test if sqlite3_step runs successful */
  s = sqlite3_step(stmt);
  if ((s != SQLITE_ROW) && (s != SQLITE_DONE) && (s != SQLITE_OK)) {
    dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));
    return;
  }
  sqlite3_reset(stmt);
  db_info->stmt = stmt;
  dbh->status = status_cons(0, "query ok");
}

SCM __sqlite3_getrow_g_db_handle(gdbi_db_handle_t *dbh)
{
  gdbi_sqlite3_ds_t *db_info = dbh->db_info;
  if (db_info == NULL) {
    dbh->status = status_cons(1, "invalid dbi connection");
    return(SCM_BOOL_F);
  }
  if (db_info->stmt == NULL) {
    dbh->status = status_cons(1, "missing query result");
    return(SCM_BOOL_F);
  }
  SCM res_row = SCM_EOL;
  SCM cur_val;
  char s = sqlite3_step(db_info->stmt);
  /* row to scheme list */
  if (s == SQLITE_ROW) {
    int col_count = sqlite3_column_count(db_info->stmt);
    int cur_col_idx = 0;
    char col_type;
    while(cur_col_idx < col_count) {
      col_type = sqlite3_column_type(db_info->stmt, cur_col_idx);
      if (col_type == SQLITE_INTEGER) {
        cur_val = scm_from_long(sqlite3_column_int(db_info->stmt,
            cur_col_idx));
      } else if (col_type == SQLITE_FLOAT) {
        cur_val = scm_from_double(sqlite3_column_double(db_info->stmt,
            cur_col_idx));
      } else if (col_type == SQLITE_TEXT) {
        cur_val = scm_from_locale_string(sqlite3_column_text(db_info->stmt,
            cur_col_idx));
      } else if (col_type == SQLITE_BLOB) {
        SCM blob_size = scm_from_int32(sqlite3_column_bytes(db_info->stmt,
            cur_col_idx));
        cur_val = scm_make_u8vector(blob_size, 0);
        if (blob_size > 0) {
          char *blob = (char *)sqlite3_column_blob(db_info->stmt, cur_col_idx);
          scm_t_array_handle array_handle;
          size_t val_size, i; ssize_t val_step;
          scm_t_uint8 *elt = scm_u8vector_writable_elements(cur_val,
            &array_handle,
            &val_size,
            &val_step);
          for (i = 0; i < val_size; i++, elt += val_step)
            *elt = *(blob + i);
          scm_array_handle_release(&array_handle);
        }
      } else if (col_type == SQLITE_NULL) {
        cur_val = SCM_BOOL_F;
      } else {
        dbh->status = status_cons(1, "unknown field type");
        return(SCM_EOL);
      }
      res_row
        = scm_append(scm_list_2(res_row,
            scm_list_1(scm_cons(scm_from_locale_string(sqlite3_column_name(db_info->stmt,
                    cur_col_idx)),
                cur_val))));
      cur_col_idx++;
    }
  } else if (s == SQLITE_DONE) {
    dbh->status = status_cons(1, "no more rows to get");
    return(SCM_BOOL_F);
  } else {
    dbh->status = status_cons(1, sqlite3_errmsg(db_info->sqlite3_obj));
    /* probably "unknown error" */
    sqlite3_finalize(db_info->stmt);
    db_info->stmt = NULL;
    return(SCM_BOOL_F);
  }
  dbh->status = status_cons(0, "row fetched");
  return(res_row);
}
