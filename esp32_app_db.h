#ifndef __ESP32_APP_DB_H___
#define __ESP32_APP_DB_H___

#define module_data_table 0
#define event_data_table 1
#define command_data_table 2
#define event_trigger_data_table 3
#define trigger_op_data_table 4
#define area_data_table 5

#define EQUAL 0
#define LIKE 1

// Storage transactions for system data

struct app_module_data_t {
    int mod_id=0;
    module_data_t module_data [2];
    unsigned char mac [6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int network_id=0;
    int area_id=0;
    String name="";
    time_t last_update=0;
};

struct app_event_data_t {
    int event_id;
    bool if_public;
    String name;
    time_t last_update;
};

struct app_command_data_t {
    int command_id;
    module_transaction command_data;
    int mod_id;
};

struct app_event_trigger_data_t {
    int ev_trg_id;
    event_trigger_transaction_t ev_trg_data;
    int mod_id;
};

struct app_trigger_operation_data_t {
    int op_id;
    int ev_out;
    combiner_transaction handle_1;
    int op;
    combiner_transaction handle_2;
    int mod_id;
};

struct app_area_data_t {
    int area_id;
    String name;
    int area_master_mod_id;
};

static int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    for (i = 0; i<argc; i++){
        Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    proccess_query_rsp();
    return 0;
}

class app_db {

    private:

    // Array names for proccessing queries
    static String tables [6] = {"mod_data", "event_data", "com_data", "ev_trg", "ev_op", "ar_data"};
    // Filter types for proccessing queries
    static String filter_types [2] = {"=", "LIKE"};

    Preferences pref;
    sqlite3 * db;

    const char* data = "Callback function called";
    char *zErrMsg = 0;
    char * query_rsp;

    // Function to change parameters into a query
    bool select_query_prepare(int table, sqlite3_stmt ** q, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        if (filter_type != -1)
            query = "SELECT * FROM " + tables[table] + " WHERE " + filter_param + " " + filter_types[filter_type] + " '" + param_value + "';";   
        else {
            query = "SELECT * FROM " + tables[table] + ";";
        }
        if(db_open("/littlefs/app.db", &db)) {
            return false;
        }
        if (sqlite3_prepare_v2(db, query.c_str(), -1, q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return false;
        }
        return true;
    }

    // DB operation handlers
    int db_open(const char *filename, sqlite3 **db) {
       int rc = sqlite3_open(filename, db);
       if (rc) {
           Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
           return rc;
       } else {
           Serial.printf("Opened database successfully\n");
       }
       return rc;
    }

    int db_exec(sqlite3 *db, const char *sql) {
       Serial.println(sql);
       int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
       if (rc != SQLITE_OK) {
           Serial.printf("SQL error: %s\n", zErrMsg);
           sqlite3_free(zErrMsg);
           Serial.println(sqlite3_extended_errcode(db));
       } else {
           Serial.printf("Operation done successfully\n");
       }
       return rc;
    }
    
    public:

    void insert_mod_data(app_module_data_t t) {
        String query = "INSERT INTO mod_data (module_data, mac, network_id, area_id, name) VALUES( ?, ?, " + (String)t.network_id + ", " + (String)t.area_id + ", '" + t.name + "');";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.module_data, sizeof(module_data_t[2]), SQLITE_TRANSIENT);
        sqlite3_bind_blob(q, 2, &t.mac, sizeof(unsigned char[6]), SQLITE_TRANSIENT);
        sqlite3_step(q);
        Serial.println("Insert mod data");
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void update_mod_data(app_module_data_t t) {
        String query = "UPDATE mod_data SET mac = ?, network_id = " + (String)t.network_id + ", area_id = " + (String)t.area_id + ", name = '" + t.name + "' WHERE id = " + (String)t.mod_id + ";";
        Serial.println(query);
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.mac, sizeof(unsigned char[6]), SQLITE_TRANSIENT);
        sqlite3_step(q);
        Serial.println("Update mod data");
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void update_mod_module_data(app_module_data_t t) {
        String query = "UPDATE mod_data SET module_data = ? WHERE id = "+ (String)t.mod_id +";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.module_data, sizeof(module_data_t[2]), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
        Serial.println("Update mod module data");
    }

    void pull_mod_data_by_parameter (app_module_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(module_data_table, &q, filter_type, filter_param, param_value)) return;
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].mod_id = sqlite3_column_int(q, 0);
            memcpy(&target[i].module_data, sqlite3_column_blob(q, 1), sizeof(module_data_t[2]));
            memcpy(&target[i].mac, sqlite3_column_blob(q, 2), sizeof(unsigned char[6]));
            target[i].network_id = sqlite3_column_int(q, 3);
            target[i].area_id = sqlite3_column_int(q, 4);
            target[i].name = (char*)sqlite3_column_text(q, 5);
            target[i].last_update = sqlite3_column_double(q, 6);
        }
        Serial.println("pull mod data");
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void insert_ev_data(app_event_data_t t) {
        String query = "INSERT INTO event_data (if_public, name) VALUES(" + (String)t.if_public + ", '" + t.name + "');";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        db_exec(db, query.c_str());
        sqlite3_close(db);
    }

    void update_ev_data(app_event_data_t t) {
        String query = "UPDATE event_data SET name = '" + t.name + "' WHERE id = " + t.event_id + ";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        db_exec(db, query.c_str());
        sqlite3_close(db);
    }

    void pull_ev_data_by_parameter (app_event_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(event_data_table, &q, filter_type, filter_param, param_value)) {return;}
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].event_id = sqlite3_column_int(q, 0);
            target[i].if_public = sqlite3_column_int(q, 1);
            target[i].name = (char*)sqlite3_column_text(q, 2);
            target[i].last_update = sqlite3_column_double(q, 3);
        }
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void insert_com_data(app_command_data_t t) {
        String query = "INSERT INTO com_data (command_data, event_id, mod_id) VALUES ( ? , " + (String)t.command_data.event_id + ", " + (String)t.mod_id + ");";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.command_data, sizeof(module_transaction), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void update_com_data(app_command_data_t t) {
        String query = "UPDATE com_data SET command_data = ? WHERE id = " + (String)t.command_id + ";";;
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.command_data, sizeof(module_transaction), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void pull_com_data_by_parameter (app_command_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(command_data_table, &q, filter_type, filter_param, param_value)) {return;}
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].command_id = sqlite3_column_int(q, 0);
            memcpy(&target[i].command_data, sqlite3_column_blob(q, 1), sizeof(module_transaction));
            target[i].command_data.event_id = sqlite3_column_int(q, 2);
            target[i].mod_id = sqlite3_column_int(q, 3);
        }
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void insert_trg_data(app_event_trigger_data_t t) {
        String query = "INSERT INTO ev_trg (trg_data, event_id, mod_id) VALUES ( ? , " + (String)t.ev_trg_data.event_id + ", " + (String)t.mod_id + ");";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.ev_trg_data, sizeof(event_trigger_transaction_t), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void update_trg_data(app_event_trigger_data_t t) {
        String query = "UPDATE ev_trg SET trg_data = ? , event_id = " + (String)t.ev_trg_data.event_id + " WHERE id = " + (String)t.ev_trg_id + ";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.ev_trg_data, sizeof(event_trigger_transaction_t), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void pull_trg_data_by_parameter (app_event_trigger_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(event_trigger_data_table, &q, filter_type, filter_param, param_value)) {return;}
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].ev_trg_id = sqlite3_column_int(q, 0);
            memcpy(&target[i].ev_trg_data, sqlite3_column_blob(q, 1), sizeof(event_trigger_transaction_t));
            target[i].mod_id = sqlite3_column_int(q, 2);
        }
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void insert_op_data(app_trigger_operation_data_t t) {
        String query = "INSERT INTO ev_op (event_id, handle_1, op_type, handle_2, mod_id) VALUES(" + (String)t.ev_out + ", ? , " + (String)t.op + ", ?, " + t.mod_id + " );";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.handle_1, sizeof(combiner_transaction), SQLITE_TRANSIENT);
        sqlite3_bind_blob(q, 2, &t.handle_2, sizeof(combiner_transaction), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void update_op_data(app_trigger_operation_data_t t) {
        String query = "UPDATE ev_op SET event_id = "+ (String)t.ev_out + ", handle_1 = ? , op_type = " + (String)t.op + ", handle_2 = ? WHERE id = " + (String)t.op_id +";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        sqlite3_stmt * q;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        sqlite3_bind_blob(q, 1, &t.handle_1, sizeof(combiner_transaction), SQLITE_TRANSIENT);
        sqlite3_bind_blob(q, 2, &t.handle_2, sizeof(combiner_transaction), SQLITE_TRANSIENT);
        sqlite3_step(q);
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void pull_op_data_by_parameter (app_trigger_operation_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(trigger_op_data_table, &q, filter_type, filter_param, param_value)) {return;}
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].op_id = sqlite3_column_int(q, 0);
            target[i].ev_out = sqlite3_column_int(q, 1);
            memcpy(&target[i].handle_1, sqlite3_column_blob(q, 2), sizeof(combiner_transaction));
            target[i].op = sqlite3_column_int(q, 3);
            memcpy(&target[i].handle_2, sqlite3_column_blob(q, 4), sizeof(combiner_transaction));
            target[i].mod_id = sqlite3_column_int(q, 5);
        }
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    void insert_ar_data(app_area_data_t t) {
        String query = "INSERT INTO ar_data (mod_id, name) VALUES (" + (String)t.area_master_mod_id + ", '" + t.name + "');";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        db_exec(db, query.c_str());
        sqlite3_close(db);
    }

    void update_ar_data(app_area_data_t t) {
        String query = "UPDATE ar_data SET mod_id = " + (String)t.area_master_mod_id + ", name = '" + t.name + "' WHERE id = " + (String)t.area_id + ";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        db_exec(db, query.c_str());
        sqlite3_close(db);
    }

    void pull_ar_data_by_parameter (app_area_data_t* target, int cnt, int filter_type = -1, String filter_param = "", String param_value = "") {
        String query;
        sqlite3_stmt * q;
        if(!select_query_prepare(area_data_table, &q, filter_type, filter_param, param_value)) {return;}
        for (int i = 0; i < cnt; i++) {
            sqlite3_step(q);
            target[i].area_id = sqlite3_column_int(q, 0);
            target[i].name = (char*)sqlite3_column_text(q, 1);
            target[i].area_master_mod_id = sqlite3_column_int(q, 2);
        }
        sqlite3_finalize(q);
        sqlite3_close(db);
    }

    int pull_cnt_by_parameter (int table, int filter_type = -1, String filter_param = "", String param_value = "") {
        int cnt;
        String query;
        if (filter_type != -1)
            query = "SELECT COUNT(1) FROM " + tables[table] + " WHERE " + filter_param + " " + filter_types[filter_type] + " '" + param_value + "';";   
        else {
            query = "SELECT COUNT(1) FROM " + tables[table];
        }
        sqlite3_stmt * q;
        if(db_open("/littlefs/app.db", &db)) {
            return 0;
        }
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &q, NULL) != SQLITE_OK) {
            sqlite3_close(db);
            return 0;
        }
        sqlite3_step(q);
        cnt = sqlite3_column_int(q, 0);
        sqlite3_finalize(q);
        sqlite3_close(db);
        return cnt;
    }

    void delete_record ( int table, int id ) {
        String query = "DELETE FROM " + tables[table] + " WHERE id = " + String(id) + ";";
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        db_exec(db, query.c_str());
        sqlite3_close(db);
    }

    void setup () {
        pref.begin("check_db");
        bool tables_exist = pref.getBool("tables_exist");
        pref.end();
        if(db_open("/littlefs/app.db", &db)) {
            return;
        }
        if (!tables_exist) {
            db_exec(db, "CREATE TABLE mod_data (id integer primary key autoincrement not null, module_data BLOB not null, mac BLOB not null, network_id INTEGER not null, area_id INTEGER not null, name TEXT, last_update DATETIME DEFAULT CURRENT_TIMESTAMP not null);");

            db_exec(db, "CREATE TABLE event_data (id integer primary key autoincrement not null, if_public BOOLEAN not null, name TEXT not null, last_update DATETIME DEFAULT CURRENT_TIMESTAMP not null);");

            db_exec(db, "CREATE TABLE com_data (id integer primary key autoincrement not null, command_data BLOB not null, event_id INTEGER not null, mod_id INTEGER not null);");

            db_exec(db, "CREATE TABLE ev_trg (id integer primary key autoincrement not null, trg_data BLOB not null, event_id INTEGER not null, mod_id INTEGER not null);");

            db_exec(db, "CREATE TABLE ev_op (id integer primary key autoincrement not null, event_id INTEGER not null, handle_1 BLOB not null, op_type INTEGER not null, handle_2 BLOB not null, mod_id integer not null);");

            db_exec(db, "CREATE TABLE ar_data (id integer primary key autoincrement not null, name TEXT, mod_id INTEGER not null);");

            pref.putBool("tables_exist", true);
        }
        sqlite3_close(db);
    }

};

#endif // __ESP32_APP_DB_H___