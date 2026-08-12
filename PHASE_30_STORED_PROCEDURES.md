# PHASE 30: Stored Procedures & Functions

**Priority:** HIGH
**Duration:** 4 weeks
**Target Tests:** 16+
**Target LOC:** 1,000+

## Objectives
1. User-defined functions (UDF)
2. Stored procedures with parameters
3. Control flow (IF/ELSE, LOOP, WHILE)
4. Function returns (scalar, table-valued)
5. Procedure execution and error handling

---

## WEEK 1-2: User-Defined Functions

### What to Build
```sql
CREATE FUNCTION calculate_discount(price DECIMAL, discount_pct DECIMAL)
RETURNS DECIMAL AS $$
BEGIN
  RETURN price * (1 - discount_pct / 100);
END;
$$ LANGUAGE plpgsql;

SELECT calculate_discount(100, 10);  -- Returns 90
```

### Implementation
- Function definition parsing
- Parameter handling
- Return type specification
- Function invocation
- Result caching

### Files to Create
- `include/dbx4/function_engine.h`
- `src/function_executor.cpp`
- `tests/test_phase30_functions.cpp` (8+ tests)

---

## WEEK 2-3: Stored Procedures

### What to Build
```sql
CREATE PROCEDURE transfer_funds(
  FROM_ACCOUNT INT,
  TO_ACCOUNT INT,
  AMOUNT DECIMAL
) AS $$
BEGIN
  UPDATE accounts SET balance = balance - AMOUNT WHERE id = FROM_ACCOUNT;
  UPDATE accounts SET balance = balance + AMOUNT WHERE id = TO_ACCOUNT;
  COMMIT;
END;
$$ LANGUAGE plpgsql;

CALL transfer_funds(1, 2, 100);
```

### Implementation
- Procedure definition parsing
- Multiple statements in procedure
- Transaction control
- Error handling (TRY/CATCH)
- Procedure invocation

### Files to Create
- `include/dbx4/procedure_engine.h`
- `src/procedure_executor.cpp`
- `tests/test_phase30_procedures.cpp` (8+ tests)

---

## ACCEPTANCE CRITERIA

- [x] All 43 existing tests still passing
- [x] 16+ new tests passing
- [x] Functions with multiple parameters working
- [x] Procedures with DML statements working
- [x] Error handling functional
- [x] Transaction control in procedures
- [x] No compilation errors

