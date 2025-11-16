-- pg_ai_ext--1.0.0.sql

-- Create the SQL function
CREATE OR REPLACE FUNCTION pg_gen_query(prompt TEXT)
RETURNS TEXT AS 'MODULE_PATHNAME','pg_gen_query'
LANGUAGE C STRICT;

-- Grant execute permission to public (change to appropriate roles if needed)
GRANT EXECUTE ON FUNCTION pg_gen_query(prompt TEXT) TO PUBLIC;