#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "access/htup.h"
#include "access/tupdesc.h"
#include "lib/stringinfo.h"
#include "executor/spi.h"
#include "utils/elog.h"
#include "utils/palloc.h"
#include <_string.h>
#include <string.h>
#include <stdbool.h>

PG_MODULE_MAGIC;

#ifdef __cplusplus
extern "C" {
#endif
    const char* ai_generate_text(const char* prompt);
    char* list_databases(void);
    char* list_tables(void);
    char* get_schema_for_table(const char* table);
#ifdef __cplusplus
}
#endif

PG_FUNCTION_INFO_V1(pg_gen_query);


/*
* List all databases
* return array of database names 
*/
char*
list_databases(void) {
    if(SPI_connect() != SPI_OK_CONNECT)
      elog(ERROR, "list_databases:SPI_connect failed");

    const char* query = 
       "SELECT COALESCE(json_agg(datname)::text, '[]') FROM (SELECT datname FROM pg_database WHERE datistemplate = false ORDER BY datname) q";
    int ret = SPI_execute(query, true, 0);
    if(ret != SPI_OK_SELECT)
    {
      SPI_finish();
      elog(ERROR, "list_databases:SPI_execute failed with code %d: %s",ret, SPI_result_code_string(ret));
    }
    if(SPI_processed == 0){
      SPI_finish();
      return strdup("[]");
    }
    TupleDesc tupdesc = SPI_tuptable->tupdesc;
    SPITupleTable *tuptable = SPI_tuptable;
    HeapTuple tuple = tuptable->vals[0];
    char* result = SPI_getvalue(tuple, tupdesc, 1);
    if (result == NULL)
    {
        elog(WARNING, "list_databases: SPI_getvalue returned NULL");
        SPI_finish();
        return strdup("[]");
    }
    char* json_result = strdup(result);
    SPI_finish();
    return json_result;
}

/*
* List all tables in a database
*/
char*
list_tables(void) {
    if(SPI_connect() != SPI_OK_CONNECT)
      elog(ERROR, "list_tables:SPI_connect failed");

    const char* query = "SELECT COALESCE(json_agg(format('%s.%s', n.nspname, c.relname))::text, '[]') "
        "FROM pg_class c "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "WHERE c.relkind IN ('r','v','p') "
        "  AND n.nspname NOT IN ('pg_catalog','information_schema','pg_toast') "
        "  AND pg_catalog.pg_table_is_visible(c.oid)";
    int ret = SPI_execute(query, true, 0);
    if(ret != SPI_OK_SELECT) {
      SPI_finish();
      elog(ERROR, "list_tables:SPI_execute failed");
    }
    if(SPI_processed == 0){
        SPI_finish();
        return strdup("[]");
    }

    HeapTuple tuple = SPI_tuptable->vals[0];
    char* json = SPI_getvalue(tuple, SPI_tuptable->tupdesc, 1);
    char* json_result = strdup(json? json : "[]");
    SPI_finish();
    return json_result;
}

/*
* Get full schema definition for a table
*/
char*
get_schema_for_table(const char* table) {
    if(SPI_connect() != SPI_OK_CONNECT)
      elog(ERROR, "list_tables:SPI_connect failed");

    char query[2048];

    snprintf(query, sizeof(query),
        "WITH cols AS ("
  "SELECT n.nspname,c.relname,a.attnum,a.attname,"
    "pg_catalog.format_type(a.atttypid, a.atttypmod) AS coltype,"
    "pg_get_expr(ad.adbin, ad.adrelid) AS coldefault,a.attnotnull"
  " FROM pg_class c JOIN pg_namespace n ON n.oid = c.relnamespace"
  " JOIN pg_attribute a ON a.attrelid = c.oid"
  " LEFT JOIN pg_attrdef ad ON ad.adrelid = c.oid AND ad.adnum = a.attnum"
  " WHERE c.relkind = 'r' AND a.attnum > 0 AND NOT a.attisdropped"
    " AND c.relname = %s)"
" SELECT 'CREATE TABLE ' || nspname || '.' || relname || E' (' ||"
  "string_agg(' ' || attname || ' ' || coltype ||"
    "COALESCE(' DEFAULT ' || coldefault, '') ||"
    "CASE WHEN attnotnull THEN ' NOT NULL' ELSE '' END"
  ", E',') || E');' AS create_table_sql"
" FROM cols GROUP BY nspname, relname",
         quote_literal_cstr(table));
    int ret = SPI_execute(query, true, 0);
    if(ret != SPI_OK_SELECT){
        SPI_finish();
        elog(ERROR, "get_schema_for_table:SPI_execute failed");
    }

    if(SPI_processed == 0){
        SPI_finish();
        return strdup("{\"create_table\":\"Table not found\"}");
    }
    char* result;
    HeapTuple tuple = SPI_tuptable->vals[0];
    char* ddl = SPI_getvalue(tuple, SPI_tuptable->tupdesc, 1);
    result = strdup(ddl);
    SPI_finish();
    return result;
}

Datum
pg_gen_query(PG_FUNCTION_ARGS) {
    text *prompt_text = PG_GETARG_TEXT_P(0);
    const char* prompt = text_to_cstring(prompt_text);
    const char* result = ai_generate_text(prompt);
    text* out = cstring_to_text(result);
    free((void*)result);
    PG_RETURN_TEXT_P(out);
}