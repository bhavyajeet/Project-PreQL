#include "global.h"
/**
 * @brief 
 * SYNTAX: R <- SELECT column_name bin_op [column_name | int_literal] FROM relation_name
 */
bool syntacticParseSELECTION()
{
    logger.log("syntacticParseSELECTION");
    if (tokenizedQuery.size() != 8 || tokenizedQuery[6] != "FROM")
    {
        cout << "SYNTAC ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = SELECTION;
    parsedQuery.selectionResultRelationName = tokenizedQuery[0];
    parsedQuery.selectionFirstColumnName = tokenizedQuery[3];
    parsedQuery.selectionRelationName = tokenizedQuery[7];

    string binaryOperator = tokenizedQuery[4];
    if (binaryOperator == "<")
        parsedQuery.selectionBinaryOperator = LESS_THAN;
    else if (binaryOperator == ">")
        parsedQuery.selectionBinaryOperator = GREATER_THAN;
    else if (binaryOperator == ">=" || binaryOperator == "=>")
        parsedQuery.selectionBinaryOperator = GEQ;
    else if (binaryOperator == "<=" || binaryOperator == "=<")
        parsedQuery.selectionBinaryOperator = LEQ;
    else if (binaryOperator == "==")
        parsedQuery.selectionBinaryOperator = EQUAL;
    else if (binaryOperator == "!=")
        parsedQuery.selectionBinaryOperator = NOT_EQUAL;
    else
    {
        cout << "SYNTAC ERROR" << endl;
        return false;
    }
    regex numeric("[-]?[0-9]+");
    string secondArgument = tokenizedQuery[5];
    if (regex_match(secondArgument, numeric))
    {
        parsedQuery.selectType = INT_LITERAL;
        parsedQuery.selectionIntLiteral = stoi(secondArgument);
    }
    else
    {
        parsedQuery.selectType = COLUMN;
        parsedQuery.selectionSecondColumnName = secondArgument;
    }
    return true;
}

bool semanticParseSELECTION()
{
    logger.log("semanticParseSELECTION");

    if (tableCatalogue.isTable(parsedQuery.selectionResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }

    if (!tableCatalogue.isTable(parsedQuery.selectionRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    if (!tableCatalogue.isColumnFromTable(parsedQuery.selectionFirstColumnName, parsedQuery.selectionRelationName))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    if (parsedQuery.selectType == COLUMN)
    {
        if (!tableCatalogue.isColumnFromTable(parsedQuery.selectionSecondColumnName, parsedQuery.selectionRelationName))
        {
            cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
            return false;
        }
    }
    return true;
}

bool evaluateBinOp(int value1, int value2, BinaryOperator binaryOperator)
{
    switch (binaryOperator)
    {
    case LESS_THAN:
        return (value1 < value2);
    case GREATER_THAN:
        return (value1 > value2);
    case LEQ:
        return (value1 <= value2);
    case GEQ:
        return (value1 >= value2);
    case EQUAL:
        return (value1 == value2);
    case NOT_EQUAL:
        return (value1 != value2);
    default:
        return false;
    }
}

void executeSELECTION()
{
    logger.log("executeSELECTION");

    cout << "select ??" << endl;

    Table * table = tableCatalogue.getTable(parsedQuery.selectionRelationName);
    Table *resultantTable = new Table(parsedQuery.selectionResultRelationName, table->columns);
    table->checkIndex();
    bool indflag = 1;
    cout << table->indexedColumn << " " << table->tableName << " "<< parsedQuery.selectionFirstColumnName << endl;
    cout << table->indexed << ": bool val" << endl;
    if (table->indexed && parsedQuery.selectType != COLUMN && table->indexedColumn == parsedQuery.selectionFirstColumnName)
    {
        cout << "using some index" << endl;
        int firstColumnIndex = table->getColumnIndex(parsedQuery.selectionFirstColumnName);
        if (table->indexingStrategy == BTREE && parsedQuery.selectionBinaryOperator != NOT_EQUAL)
        {
            int valCon = parsedQuery.selectionIntLiteral;
            cout << "TRYING TO GET FROM BPTREE " << endl;
            pair<int, int> address = table->BplusTree.search(valCon);
            int page = address.first;
            int row = address.second;
            if (parsedQuery.selectionBinaryOperator == EQUAL)
            {
                bool eqflag = 0; 
                Cursor curs(table->tableName, page);
                vector<int> row = curs.getNext();
                while (!row.empty())
                {
                    if ( row[firstColumnIndex] ==  valCon )
                    {
                        eqflag =1;
                        resultantTable->writeRow<int>(row);
                    }
                    else if ( row[firstColumnIndex] !=  valCon && eqflag){
                        break;
                    }

                    row = curs.getNext();
                }
            }
            else if (parsedQuery.selectionBinaryOperator == LESS_THAN)
            {
            }
            else if (parsedQuery.selectionBinaryOperator == GREATER_THAN)
            {
            }
            else if (parsedQuery.selectionBinaryOperator == LEQ)
            {
            }
            else if (parsedQuery.selectionBinaryOperator == GEQ)
            {
            }
        }
        else if (table->indexingStrategy == HASH && parsedQuery.selectionBinaryOperator == EQUAL)
        {
            cout << "TRYING TO GET FROM HASH " << endl;
            indflag = 0;
            table->Hashing.searchElement(parsedQuery.selectionIntLiteral);
        }
    }

    // no index can be used
    if (indflag == 1)
    {
        cout << "NO INDEX" << endl; 
        Cursor cursor = table->getCursor();
        vector<int> row = cursor.getNext();
        int firstColumnIndex = table->getColumnIndex(parsedQuery.selectionFirstColumnName);
        int secondColumnIndex;
        if (parsedQuery.selectType == COLUMN)
            secondColumnIndex = table->getColumnIndex(parsedQuery.selectionSecondColumnName);
        while (!row.empty())
        {
            cout << "NO INDEX" << endl; 
            int value1 = row[firstColumnIndex];
            int value2;
            if (parsedQuery.selectType == INT_LITERAL)
                value2 = parsedQuery.selectionIntLiteral;
            else
                value2 = row[secondColumnIndex];
            if (evaluateBinOp(value1, value2, parsedQuery.selectionBinaryOperator))
                resultantTable->writeRow<int>(row);
            row = cursor.getNext();
        }
        if (resultantTable->blockify())
            tableCatalogue.insertTable(resultantTable);
        else
        {
            cout << "Empty Table" << endl;
            delete resultantTable;
        }
        return;
    }
}