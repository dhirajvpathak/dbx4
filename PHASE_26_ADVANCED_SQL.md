# PHASE 26: Advanced SQL Features

**Priority:** HIGH
**Duration:** 8 weeks
**Target Tests:** 50+
**Target LOC:** 2,000+

## Objectives
1. INNER/OUTER JOINs (all types)
2. Aggregate functions (COUNT, SUM, AVG, MIN, MAX)
3. GROUP BY / HAVING clauses
4. Subqueries & CTEs (WITH clause)
5. Window functions (ROW_NUMBER, RANK, LAG, LEAD)

---

## WEEK 1-2: JOIN Operations

### What to Build
```cpp
// Support all JOIN types:
SELECT u.name, o.amount
FROM users u
INNER JOIN orders o ON u.id = o.user_id;

SELECT u.name, COUNT(o.id) as order_count
FROM users u
LEFT JOIN orders o ON u.id = o.user_id
GROUP BY u.id;
```

### Implementation Steps
1. Extend SQL parser to recognize JOIN syntax
2. Implement HashJoin executor
3. Implement NestedLoopJoin executor
4. Implement SortMergeJoin executor
5. Optimize join order selection

### Files to Create
- `include/dbx4/join_engine.h`
- `src/join_executor.cpp`
- `tests/test_phase26_joins.cpp` (12+ tests)

---

## WEEK 3-4: Aggregate Functions

### What to Build
```cpp
SELECT 
  category,
  COUNT(*) as total,
  SUM(amount) as revenue,
  AVG(amount) as avg_amount,
  MIN(amount) as min_amount,
  MAX(amount) as max_amount
FROM products
GROUP BY category
HAVING SUM(amount) > 1000;
```

### Implementation Steps
1. Extend parser for GROUP BY / HAVING
2. Implement aggregate function framework
3. Implement COUNT, SUM, AVG, MIN, MAX
4. Implement group-by execution
5. Implement HAVING filter

### Files to Create
- `include/dbx4/aggregate_engine.h`
- `src/aggregate_executor.cpp`
- `tests/test_phase26_aggregates.cpp` (12+ tests)

---

## WEEK 5-6: Subqueries & CTEs

### What to Build
```cpp
// Scalar subquery
SELECT name, (SELECT COUNT(*) FROM orders WHERE user_id = users.id) as order_count
FROM users;

// CTE
WITH active_users AS (
  SELECT id FROM users WHERE status = 'active'
)
SELECT * FROM orders WHERE user_id IN (SELECT id FROM active_users);
```

### Implementation Steps
1. Parse subqueries (SELECT ... FROM (...))
2. Implement scalar subquery execution
3. Implement IN / EXISTS subqueries
4. Parse WITH (CTE) syntax
5. Implement CTE resolution

### Files to Create
- `include/dbx4/subquery_engine.h`
- `src/subquery_executor.cpp`
- `tests/test_phase26_subqueries.cpp` (12+ tests)

---

## WEEK 7-8: Testing & Polish

### Tasks
1. Window functions (basic ROW_NUMBER)
2. Performance optimization (< 5ms latency)
3. All 50+ tests passing
4. Documentation complete
5. Code review

### Acceptance Criteria
- [x] All 34 existing tests still passing
- [x] 50+ new tests passing
- [x] JOINs: < 5ms P99 for 3-table joins
- [x] Aggregates: < 2ms for small datasets
- [x] Subqueries: < 3ms for non-correlated
- [x] Zero compiler warnings
- [x] Full documentation

