# PHASE 26-27 Continuation Plan

## PHASE 26: Advanced SQL (Week 5-8) - IN PROGRESS

### Completed ✅
- JOINs (INNER, LEFT, RIGHT, FULL, CROSS)
- Aggregates (COUNT, SUM, AVG, MIN, MAX)

### Next: Subqueries & CTEs (Week 5-6)

**What to Build:**
- Scalar subqueries: SELECT (SELECT COUNT(*) FROM orders) AS count
- IN/EXISTS subqueries: WHERE id IN (SELECT id FROM ...)
- WITH clauses (CTEs): WITH temp AS (...) SELECT FROM temp
- Recursive CTEs

**Target:** 12+ tests, < 3ms latency

### Then: Window Functions (Week 7-8)

**What to Build:**
- ROW_NUMBER() OVER (ORDER BY col)
- RANK() / DENSE_RANK()
- LAG() / LEAD()
- FIRST_VALUE() / LAST_VALUE()

**Target:** 12+ tests, all window functions working

---

## PHASE 27: Indexing & Query Optimization (Week 2-5) - IN PROGRESS

### Completed ✅
- B-tree indexes (single & multi-column)

### Next: Hash Indexes (Week 3)

**What to Build:**
- Hash table index for equality queries
- O(1) lookup
- Perfect for WHERE id = value

**Target:** 8+ tests

### Then: Query Optimizer (Week 4)

**What to Build:**
- Cost-based optimization
- Select best index
- Join order optimization
- Predicate pushdown

**Target:** 12+ tests

### Then: EXPLAIN Statement (Week 5)

**What to Build:**
- EXPLAIN query plans
- Cost estimates
- Execution statistics
- EXPLAIN ANALYZE

**Target:** 10+ tests

---

## Target: 100+ TESTS by end of Phase 27

Current: 39/39 ✅
Phase 26 finish: 50+ tests
Phase 27 finish: 100+ tests

