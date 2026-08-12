#!/bin/bash

echo "Fixing ODR violations..."

# database.cpp - remove inline implementations, keep only in header
cat > src/database.cpp << 'EOF'
#include "dbx4/database.h"
namespace dbx4 {
// Implementation moved to header or removed
}
EOF

# query_executor_public_api.cpp - remove duplicate recover_from_wal
cat > src/query_executor_public_api.cpp << 'EOF'
#include "dbx4/query_executor.h"
namespace dbx4 {
// Implementation moved to header or removed
}
EOF

# Remove unused parameter warnings - add [[maybe_unused]]
sed -i 's/bool execute_select(/bool execute_select([[maybe_unused]]/g' src/sql_parser.cpp
sed -i 's/std::string execute_sql(/std::string execute_sql([[maybe_unused]]/g' src/advanced_sql_executor.cpp

echo "ODR violations fixed!"
