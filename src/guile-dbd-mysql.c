/* guile-dbd-mysql.c - Main source file
 * Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
 * Written by Maurizio Boriani <baux@member.fsf.org>
 *
 * This file is part of the guile-dbd-mysql.
 * 
 * The guile-dbd-mysql is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * The guile-dbd-mysql is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * Process this file with autoconf to produce a configure script. */


#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <guile/gh.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>



/* functions prototypes and structures */
void __mysql_make_g_db_handle(gdbi_db_handle_t* dbh);
void __mysql_close_g_db_handle(gdbi_db_handle_t* dbh);
void __mysql_query_g_db_handle(gdbi_db_handle_t* dbh, char* query);
SCM __mysql_getrow_g_db_handle(gdbi_db_handle_t* dbh);


typedef struct gdbi_mysql_ds
{
  MYSQL* mysql;
  MYSQL_RES* res;
  int retn;
} gdbi_mysql_ds_t;



void
__mysql_make_g_db_handle(gdbi_db_handle_t* dbh)
{
  char sep=':';
  int  items=0;
  gdbi_mysql_ds_t* mysqlP = NULL;
  SCM cp_list = SCM_EOL;


  if(scm_equal_p(scm_string_p(dbh->constr), SCM_BOOL_F) == SCM_BOOL_T)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("missing connection string"));
      return;
    }

  cp_list = scm_string_split(dbh->constr,SCM_MAKE_CHAR(sep));

  dbh->closed = SCM_BOOL_T;
  
  items=SCM_INUM(scm_length(cp_list));
  if (items >= 5 && items < 8)
    {
      int ret = 0;
      int port = 0;
      dbh->db_info = (MYSQL*) mysql_init(NULL);
      char* user = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(0)),NULL);
      char* pass = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(1)),NULL);
      char* db   = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(2)),NULL);
      char* ctyp = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(3)),NULL);
      char* loc  = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(4)),NULL);

      mysqlP = (gdbi_mysql_ds_t*)malloc(sizeof(gdbi_mysql_ds_t));
      mysqlP->retn = 0;

      if (mysqlP == NULL)
	{
	  dbh->status = scm_cons(SCM_MAKINUM(errno),
				 scm_makfrom0str(strerror(errno)));
	  return;
	}

      mysqlP->mysql = mysql_init(NULL);
      mysqlP->res   = NULL;

      if (strcmp(ctyp,"tcp") == 0)
	{
	  char* sport  = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(5)),
				       NULL);
	  port = atoi(sport);
	  ret = (int) mysql_real_connect(mysqlP->mysql, loc, user, pass, db, 
					 port, NULL, 0);
	  if (items == 7)
	    {
	      char* sretn  = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(6)),
					   NULL);
	      mysqlP->retn = atoi(sretn);
	    }
	}
      else
	{
	  ret = (int) mysql_real_connect(mysqlP->mysql, NULL, user, pass, db, port, loc, 
					 0);
	  if (items == 6)
	    {
	      char* sretn  = gh_scm2newstr(scm_list_ref(cp_list,SCM_MAKINUM(5)),
					   NULL);
	      mysqlP->retn = atoi(sretn);
	    }
	}

      /* free resources */
      if (user)
	{
	  free(user);
	}
      if (pass)
	{
	  free(pass);
	}
      if (db)
	{
	  free(db);
	}
      if (ctyp)
	{
	  free(ctyp);
	}
      if (loc)
	{
	  free(loc);
	}

      if(ret == 0)
	{
	  dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				       scm_makfrom0str(mysql_error(mysqlP->mysql)));
	  mysql_close(mysqlP->mysql);
	  mysqlP->mysql = NULL;
	  free(mysqlP);
	  dbh->db_info=NULL;
	  return;
	}
      else
	{
	  /* todo: error msg to be translated */
	  dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
				       scm_makfrom0str("db connected"));
	  dbh->db_info = mysqlP;
	  dbh->closed = SCM_BOOL_F;
	  return;
	}
    }
  else
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("invalid connection string"));
      dbh->db_info = NULL;
      return;
    }

  return;
}



void 
__mysql_close_g_db_handle(gdbi_db_handle_t* dbh)
{
  gdbi_mysql_ds_t* mysqlP = (gdbi_mysql_ds_t*)dbh->db_info;  
  if (mysqlP == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("dbd info not found"));
      return;
    }
  else if (mysqlP->mysql == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("dbi connection already closed"));
      free(dbh->db_info);
      dbh->db_info = NULL;
      return;
    }

  mysql_close(mysqlP->mysql);
  if (mysqlP->res)
    {
      mysql_free_result(mysqlP->res);
      mysqlP->res = NULL;
    }

  free(dbh->db_info);
  dbh->db_info = NULL;

  /* todo: error msg to be translated */
  dbh->closed = SCM_BOOL_T;
  dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
			       scm_makfrom0str("dbi closed"));
  return;

}



void
__mysql_query_g_db_handle(gdbi_db_handle_t* dbh, char* query)
{
  gdbi_mysql_ds_t* mysqlP = NULL;
  int err,i;

  if(dbh->db_info == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("invalid dbi connection"));
      return;
    }

  mysqlP = (gdbi_mysql_ds_t*)dbh->db_info;

  if (mysqlP->res)
    {
      mysql_free_result(mysqlP->res);
      mysqlP->res = NULL;
    }

  err = mysql_real_query(mysqlP->mysql, query, strlen(query));
  for (i = 0; i < mysqlP->retn && err != 0; i++) 
    {
      mysql_ping (mysqlP->mysql);
      err = mysql_real_query(mysqlP->mysql, query, strlen(query));
    }

  if (err)
    {
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str(mysql_error(mysqlP->mysql)));
      return;
    }

  mysqlP->res = mysql_use_result(mysqlP->mysql);
  if(mysqlP->res == NULL)
    {
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
				   scm_makfrom0str("query ok, no results"));
      return;
    }

  dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
			       scm_makfrom0str("query ok, got results"));

  return;
}



SCM
__mysql_getrow_g_db_handle(gdbi_db_handle_t* dbh)
{
  gdbi_mysql_ds_t* mysqlP = NULL;
  SCM retrow = SCM_EOL;
  long fnum = 0;
  long f    = 0;
  unsigned long *les;
  MYSQL_ROW row;
  MYSQL_FIELD* fields;

  if(dbh->db_info == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("invalid dbi connection"));
      return (SCM_BOOL_F);
    }

  mysqlP = (gdbi_mysql_ds_t*)dbh->db_info;

  if (!mysqlP->res)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				   scm_makfrom0str("missing query result"));
      return (SCM_BOOL_F);
    }

  if ((row = mysql_fetch_row(mysqlP->res)) == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
				   scm_makfrom0str("row end"));
      return (SCM_BOOL_F);
    }
  fnum = mysql_num_fields(mysqlP->res);
  fields = mysql_fetch_fields(mysqlP->res);
  les = mysql_fetch_lengths(mysqlP->res);
  for(f=0; f<fnum; f++)
    {

      SCM value;
      char* value_str = NULL;
      
      switch (fields[f].type)
	{
	case FIELD_TYPE_NULL:
	  value = SCM_BOOL_F;
	  break;
	case FIELD_TYPE_TINY:
	case FIELD_TYPE_SHORT:
	case FIELD_TYPE_INT24:
	case FIELD_TYPE_LONG:
	case FIELD_TYPE_LONGLONG:
	case FIELD_TYPE_DECIMAL:
	case FIELD_TYPE_FLOAT:
	case FIELD_TYPE_DOUBLE: 
	  value_str = (char*) strndup(row[f],les[f]);
	  value = scm_int2num(atoi(value_str));
	  break;
	case FIELD_TYPE_STRING:
	case FIELD_TYPE_ENUM:
	case FIELD_TYPE_VAR_STRING:
	case FIELD_TYPE_SET:
	case FIELD_TYPE_BLOB:
	case FIELD_TYPE_DATE:
	case FIELD_TYPE_TIME:
	case FIELD_TYPE_DATETIME:
	case FIELD_TYPE_TIMESTAMP:
	case FIELD_TYPE_YEAR:
	  value_str = (char*)strndup(row[f],les[f]);
	  value = (SCM)scm_makfrom0str(value_str);
	  break;
	default:
	  /* todo: error msg to be translated */
	  dbh->status = (SCM) scm_cons(SCM_MAKINUM(1),
				       scm_makfrom0str("unknown field type"));
	  return SCM_EOL;
	  break;
     	}
      retrow = scm_append (scm_list_2(retrow,
				      scm_list_1(scm_cons(scm_makfrom0str(fields[f].name),
				      value))));
      if (value_str != NULL)
	{
	  free(value_str);
	}
    }
  /* todo: error msg to be translated */
  dbh->status = (SCM) scm_cons(SCM_MAKINUM(0),
			       scm_makfrom0str("row fetched"));
  return (retrow);
}
