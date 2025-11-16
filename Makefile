MODULES = pg_ai_ext
EXTENSION = pg_ai_ext
DATA = pg_ai_ext--1.0.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)