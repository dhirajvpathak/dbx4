#include "dbx4/query_executor.h"
#include <iostream>

namespace dbx4 {

void QueryExecutor::recover_from_wal() {
    std::cout << "[QueryExecutor] Recovering from WAL\n";
}

}
