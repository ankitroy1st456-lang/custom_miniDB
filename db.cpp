#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
using namespace std;
// Column definition
struct Column
{
    string name;
    string type;
};

// Row definition
struct Row
{
    unordered_map<string, string> values;
};

// Table definition
class Table
{
public:
    string tableName;
    vector<Column> columns;
    vector<Row> rows;

    Table(const string &name) : tableName(name) {}

    void addColumn(const string &name, const string &type)
    {
        columns.push_back({name, type});
    }

    void insertRow(const Row &row)
    {
        rows.push_back(row);
    }
};

// Database manager
class Database
{
public:
    unordered_map<string, Table> tables;

    void createTable(const string &name, const vector<Column> &cols)
    {
        Table t(name);
        for (const auto &c : cols)
        {
            t.addColumn(c.name, c.type);
        }
        tables.emplace(name, std::move(t)); // ✅ no default constructor needed
        cout << "Table " << name << " created.\n";
    }

    Table *getTable(const string &name)
    {
        auto it = tables.find(name);
        if (it != tables.end())
            return &it->second;
        return nullptr;
    }
};

// Query Processor
class QueryProcessor
{
public:
    void execute(const string &query, Database &db)
    {
        stringstream ss(query);
        string command;
        ss >> command;

        if (command == "CREATE")
        {
            string temp, tableName;
            ss >> temp; // skip "TABLE"
            ss >> tableName;

            string colsDef;
            getline(ss, colsDef); // e.g. (ID int, Name string, Age int)

            // remove parentheses
            colsDef.erase(remove(colsDef.begin(), colsDef.end(), '('), colsDef.end());
            colsDef.erase(remove(colsDef.begin(), colsDef.end(), ')'), colsDef.end());

            stringstream cs(colsDef);
            string colDef;
            vector<Column> cols;

            while (getline(cs, colDef, ','))
            {
                stringstream cd(colDef);
                string name, type;
                cd >> name >> type;
                if (!name.empty() && !type.empty())
                {
                    cols.push_back({name, type});
                }
            }

            db.createTable(tableName, cols);
        }
        else if (command == "INSERT")
        {
            string temp, tableName;
            ss >> temp; // skip "INTO"
            ss >> tableName;
            ss >> temp; // skip "VALUES"

            string values;
            getline(ss, values);

            Table *table = db.getTable(tableName);
            if (!table)
            {
                cout << "Table " << tableName << " not found.\n";
                return;
            }

            Row row;
            stringstream vs(values);
            string value;
            size_t colIndex = 0;

            while (getline(vs, value, ','))
            {
                if (colIndex < table->columns.size())
                {
                    row.values[table->columns[colIndex].name] = value;
                }
                colIndex++;
            }
            table->insertRow(row);
            cout << "Row inserted into " << tableName << ".\n";
        }
        else if (command == "SELECT")
        {
            string option;
            ss >> option;

            if (option == "ALL")
            {
                string temp, tableName;
                ss >> temp; // skip "FROM"
                ss >> tableName;

                Table *table = db.getTable(tableName);
                if (!table)
                {
                    cout << "Table " << tableName << " not found.\n";
                    return;
                }

                for (const auto &row : table->rows)
                {
                    printRow(row, *table);
                }
            }
            else if (option == "WHERE")
            {
                string condition, temp, tableName;
                ss >> condition; // e.g., Age=22
                ss >> temp;      // skip "FROM"
                ss >> tableName;

                size_t pos = condition.find('=');
                if (pos == string::npos)
                {
                    cout << "Invalid SELECT WHERE syntax.\n";
                    return;
                }
                string colName = condition.substr(0, pos);
                string colValue = condition.substr(pos + 1);

                Table *table = db.getTable(tableName);
                if (!table)
                {
                    cout << "Table " << tableName << " not found.\n";
                    return;
                }

                for (const auto &row : table->rows)
                {
                    auto it = row.values.find(colName);
                    if (it != row.values.end() && it->second == colValue)
                    {
                        printRow(row, *table);
                    }
                }
            }
        }
        else if (command == "DELETE")
        {
            auto trim = [](    string s)
            {
                s.erase(0, s.find_first_not_of(" \t"));
                s.erase(s.find_last_not_of(" \t") + 1);
                return s;
            };
                string temp, condition, tableName;
            ss >> temp;      // skip "WHERE"
            ss >> condition; // e.g., ID=2
            ss >> temp;      // skip "FROM"
            ss >> tableName;

            size_t pos = condition.find('=');
            if (pos ==     string::npos)
            {
                    cout << "Invalid DELETE syntax.\n";
                return;
            }
                string colName = trim(condition.substr(0, pos));
                string colValue = trim(condition.substr(pos + 1));

            Table *table = db.getTable(tableName);
            if (!table)
            {
                    cout << "Table " << tableName << " not found.\n";
                return;
            }

            auto &rows = table->rows;
            auto newEnd =     remove_if(rows.begin(), rows.end(),
                                         [&](const Row &row)
                                         {
                                             auto it = row.values.find(colName);
                                             return (it != row.values.end() && trim(it->second) == colValue);
                                         });

            if (newEnd != rows.end())
            {
                rows.erase(newEnd, rows.end());
                    cout << "Rows deleted where " << colName << "=" << colValue
                          << " from " << tableName << ".\n";
            }
            else
            {
                    cout << "No matching rows found to delete.\n";
            }
        }
        else if (command == "UPDATE")
        {
            string tableName, temp, setClause, whereClause;
            ss >> tableName;   // table name
            ss >> temp;        // skip "SET"
            ss >> setClause;   // e.g., Age=24
            ss >> temp;        // skip "WHERE"
            ss >> whereClause; // e.g., Name=Meera

            size_t posSet = setClause.find('=');
            size_t posWhere = whereClause.find('=');
            if (posSet == string::npos || posWhere == string::npos)
            {
                cout << "Invalid UPDATE syntax.\n";
                return;
            }

            string setCol = setClause.substr(0, posSet);
            string setVal = setClause.substr(posSet + 1);
            string whereCol = whereClause.substr(0, posWhere);
            string whereVal = whereClause.substr(posWhere + 1);

            Table *table = db.getTable(tableName);
            if (!table)
            {
                cout << "Table " << tableName << " not found.\n";
                return;
            }

            for (auto &row : table->rows)
            {
                auto it = row.values.find(whereCol);
                if (it != row.values.end() && it->second == whereVal)
                {
                    row.values[setCol] = setVal;
                }
            }

            cout << "Rows updated in " << tableName << " where " << whereCol << "=" << whereVal << ".\n";
        }
        else
        {
            cout << "Unsupported query.\n";
        }
    }

private:
    void printRow(const Row &row, const Table &table)
    {
        for (size_t i = 0; i < table.columns.size(); i++)
        {
            auto it = row.values.find(table.columns[i].name);
            if (it != row.values.end())
            {
                cout << it->second;
            }
            else
            {
                cout << "NULL";
            }
            if (i < table.columns.size() - 1)
                cout << ", ";
        }
        cout << "\n";
    }
};

// Demo with runtime input
int main()
{
    Database db;
    QueryProcessor qp;

    cout << "MiniDB ready. Type commands (CREATE, INSERT, SELECT). Type EXIT to quit.\n";

    string query;
    while (true)
    {
        cout << "db> ";
        getline(cin, query);

        if (query == "EXIT")
            break;
        if (query.empty())
            continue;
        if (query == "HELP")
        {
            cout << "Available commands:\n";
            cout << "  CREATE TABLE TableName (col type, col type, ...) \n";
            cout << "      -> Create a new table with given columns\n";
            cout << "  INSERT INTO TableName VALUES val1,val2,val3 \n";
            cout << "      -> Insert a new row into the table\n";
            cout << "  SELECT ALL FROM TableName \n";
            cout << "      -> Show all rows from the table\n";
            cout << "  SELECT WHERE column=value FROM TableName \n";
            cout << "      -> Show rows matching condition\n";
            cout << "  DELETE WHERE column=value FROM TableName \n";
            cout << "      -> Delete rows matching condition\n";
            cout << "  UPDATE TableName SET col=value WHERE col=value \n";
            cout << "      -> Update rows in the table\n";
            cout << "  HELP \n";
            cout << "      -> Show this list of commands\n";
            cout << "  EXIT \n";
            cout << "      -> Quit the program\n";
            continue;
        }
        qp.execute(query, db);
    }

    return 0;
}