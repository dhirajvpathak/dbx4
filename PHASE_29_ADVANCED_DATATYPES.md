# PHASE 29: Advanced Data Types

**Priority:** HIGH
**Duration:** 4 weeks
**Target Tests:** 18+
**Target LOC:** 1,200+

## Objectives
1. JSON data type with operators
2. Array data type (int[], varchar[])
3. UUID/GUID support
4. Geospatial types (POINT, POLYGON)
5. Full-text search indexing

---

## WEEK 1-2: JSON Data Type

### What to Build
```sql
CREATE TABLE users (
  id INT,
  profile JSON
);

INSERT INTO users VALUES (1, '{"name":"Alice","age":30,"city":"NYC"}');

SELECT profile->>'name' AS name FROM users;  -- Alice
SELECT profile->'age' AS age FROM users;    -- 30
SELECT profile@>'{"city":"NYC"}' FROM users; -- true
```

### Implementation
- JSON parsing and storage
- JSON operators (->>, ->, @>, ?)
- JSON array access
- JSON key validation
- JSON indexing

### Files to Create
- `include/dbx4/json_type.h`
- `src/json_executor.cpp`
- `tests/test_phase29_json.cpp` (7+ tests)

---

## WEEK 2-3: Array Data Type

### What to Build
```sql
CREATE TABLE products (
  id INT,
  tags VARCHAR[],
  prices DECIMAL[]
);

INSERT INTO products VALUES (1, ARRAY['electronics','gadgets'], ARRAY[99.99, 149.99]);

SELECT tags[1] FROM products;  -- electronics
SELECT array_length(tags) FROM products;  -- 2
SELECT 'electronics' = ANY(tags) FROM products;  -- true
```

### Implementation
- Array type support (int[], varchar[], decimal[])
- Array indexing (1-based)
- Array functions (array_length, array_append)
- Array operators (=ANY, <ALL, >ALL)
- Array slicing

### Files to Create
- `include/dbx4/array_type.h`
- `src/array_executor.cpp`
- `tests/test_phase29_arrays.cpp` (7+ tests)

---

## WEEK 3: UUID & Geospatial

### What to Build
```sql
CREATE TABLE locations (
  id UUID PRIMARY KEY,
  position POINT,
  service_area POLYGON
);

INSERT INTO locations VALUES (
  uuid_generate_v4(),
  POINT(40.7128, -74.0060),
  POLYGON((0,0), (10,0), (10,10), (0,10))
);

SELECT * FROM locations WHERE position <-> POINT(40.7, -74.0) < 1000;  -- within 1km
```

### Implementation
- UUID type with uuid_generate_v4()
- POINT type (x, y coordinates)
- POLYGON type (multi-point geometry)
- Geospatial operators (<->, &&, @)
- Distance calculations

### Files to Create
- `include/dbx4/uuid_type.h`
- `include/dbx4/geospatial_type.h`
- `src/geospatial_executor.cpp`
- `tests/test_phase29_geospatial.cpp` (4+ tests)

---

## ACCEPTANCE CRITERIA

- [x] All 40 existing tests still passing
- [x] 18+ new tests passing
- [x] JSON parsing and operators working
- [x] Array indexing and functions working
- [x] UUID generation working
- [x] Geospatial distance calculations accurate
- [x] Proper type validation
- [x] No compilation errors

