#include "ai/openai.h"
#include "ai/tools.h"
#include "ai/types/message.h"
#include "ai/types/generate_options.h"
#include "ai/types/tool.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
using ai::JsonValue;
using ai::ToolExecutionContext;

extern "C" {
    char* list_databases(void);
    char* list_tables(void);
    char* get_schema_for_table( const char* table);
}

std::vector<std::string> get_items_list(std::string payload){
    std::vector<std::string> items;
    try {
          JsonValue json_result = JsonValue::parse(payload);
          if(json_result.is_array()){
            for (size_t i = 0; i < json_result.size(); ++i) {
                  items.push_back(json_result[i].get<std::string>());
            }
          }
      } catch (const std::exception& e){
        std::cout << "[ERROR] Failed to parse Json from '" << payload << "':" << e.what() <<"\n";
      }
      return items;
}

extern "C" const char* ai_generate_text(const char* prompt) {
    try {
           auto client = ai::openai::create_client();

            ai::ToolSet tools = {
              {"list_databases", ai::create_simple_tool("list databases", "List the databases",
                {},
                [](const JsonValue& args, const ToolExecutionContext& context)-> JsonValue{
                  std::cout << "[|] calling list_databases()...\n";
                  char* raw = list_databases();
                  if(raw == NULL){
                     std::cout << "[DEBUG] list_databases returned NULL\n";
                     return JsonValue::parse("[]");
                  }
                  std::string payload(raw);
                  free(raw);
                  std::stringstream result;
                  try {
                    auto dbs = get_items_list(payload);
                    result << " Found " << dbs.size() << " databases:\n";
                    for(const auto&db :dbs){
                      result << "- " << db << "\n";
                    }
                    std::cout << result.str();
                    return JsonValue{{"result",result.str()},
                                    {"databases", dbs}};
                  } catch (const std::exception& e){
                    std::cout << "[ERROR] Failed to parse Json from '" << payload << "':" << e.what() <<"\n";
                    return JsonValue::parse("[]");
                  }            
                })},
              {"list_tables_in_database", ai::create_simple_tool("list tables", "List tables in database", 
                {},
                [](const JsonValue& args, const ToolExecutionContext&)-> JsonValue{
                    std::cout << "[|] calling list_tables()...\n";
                    char* raw = list_tables();
                    std::string payload(raw);
                    free(raw);
                    std::stringstream result;
                    auto tables = get_items_list(payload);
                    result << " Found " << tables.size() << " tables in database"  << ":\n";
                    for(const auto& table : tables){
                      result << "- " << table << "\n";
                    }
                    std::cout << result.str();
                    if(tables.empty()){
                      result << "No tables in this database\n";
                      return ai::JsonValue{
                        {"result",result.str()}, 
                        {"tables", tables}
                            };
                      }
                    return ai::JsonValue{
                      {"result",result.str()},
                        {"tables",tables}};
                      })},
              {"get_schema_for_table", ai::create_simple_tool("get schema for table", "get schema for table", 
                {{"database_name","string"},{"table_name","string"}},
                [](const JsonValue& args, const ToolExecutionContext&)-> JsonValue{
                   std::cout << "[|] calling get_schema_for_table()...\n";
                    std::string db = args["database_name"].get<std::string>();
                    std::string tbl_name = args["table_name"].get<std::string>();
                    size_t table_pos = tbl_name.find('.');
                    if(table_pos != std::string::npos){
                      tbl_name = tbl_name.substr(table_pos +1);
                    }
                    std::cout << " arguments: database='" << db << "' table='" << tbl_name << "'\n";
                    char* raw = get_schema_for_table(tbl_name.c_str());
                    std::string schema(raw);
                    free(raw);
                    if(schema.empty()){
                      return ai::JsonValue{
                        {"result","No schema found for " + db +"."+ tbl_name},
                        {"database",db},
                        {"table",tbl_name},
                        {"schema",schema}};
                    }
                    std::cout << " Schema for " << db <<"."<< tbl_name << ": " << schema << "\n";
                    return ai::JsonValue{
                      {"result","Schema for " + db +"."+tbl_name +":\n" + schema},
                      {"database",db},
                      {"table",tbl_name},
                      {"schema",schema}};
                  })},
            };

            const std::string system_prompt = R"(You are a SQL code generator. Your ONLY job is to output SQL statements.

            TOOLS AVAILABLE:
            - list_databases(): Lists all databases
            - list_tables_in_database(database): Lists tables in a specific database  
            - get_schema_for_table(database, table): Gets schema for existing tables only

            RULES:
            1. ALWAYS output SQL ready for execution without new line characters or '+' character.
            2. NEVER ask questions or request clarification
            3. NEVER explain your SQL or add any other text
            4. For CREATE TABLE requests, ALWAYS generate a reasonable schema based on the table name

            RESPONSE FORMAT - Must be EXACTLY:
            [SQL STATEMENT]

            TASK INSTRUCTIONS:
            For "create a table named X for users":
            - Generate CREATE TABLE with columns: id BIGINT, user TEXT, created_at TIMESTAMP WITH TIME ZONE NOT NULL

            For "insert 3 rows into X table":
            - If table exists, check schema with tools
            - Generate INSERT with columns from the schema retrieved from the tools
            - Use sample data for values

            For "show all users from X" or "find all Y from X":
            - Generate appropriate SELECT statement

            IMPORTANT: Even if you use tools and find the database/table doesn't exist, still generate the SQL as requested.)";

            ai::GenerateOptions options;
            options.system = system_prompt;
            options.model = ai::openai::models::kGpt41Nano;
            options.prompt = prompt;
            options.tools = tools;
            options.max_steps = 5;
            options.temperature = 1;
            
            ai::GenerateResult result = client.generate_text(options);
            if (!result.is_success()) {
                return strdup( result.error_message().c_str());
            }
           
            std::string response_text = result.text;
            if(!result.steps.empty()){
              for(auto it = result.steps.rbegin(); it!= result.steps.rend(); ++it){
                 if(!it->text.empty()){
                    response_text = it->text;
                    break;
                 }
              }
            }
          return strdup(response_text.c_str());
    } catch (const std::exception& e) {
        return strdup(e.what());
    }
}
