-- =====================================================
-- Parking Management System - Database Schema
-- =====================================================

-- TODO: راجعوا الأنواع (VARCHAR lengths, إلخ) حسب احتياجكم الفعلي

CREATE TABLE employee (
    employee_id SERIAL PRIMARY KEY,
    username    VARCHAR(50) UNIQUE NOT NULL,
    password    VARCHAR(255) NOT NULL,
    name        VARCHAR(100) NOT NULL
);

CREATE TABLE customer (
    customer_id SERIAL PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    phone       VARCHAR(20) UNIQUE NOT NULL,
    email       VARCHAR(100)
);

CREATE TABLE vehicle (
    vehicle_id   SERIAL PRIMARY KEY,
    customer_id  INT NOT NULL REFERENCES customer(customer_id),
    plate_number VARCHAR(20) UNIQUE NOT NULL,
    model        VARCHAR(50),
    vehicle_type VARCHAR(20) NOT NULL CHECK (vehicle_type IN ('Car', 'Motorcycle'))
    -- TODO: أضف أعمدة خاصة بكل نوع لو احتجتوا (e.g. number_of_doors, engine_cc)
);

CREATE TABLE zone (
    zone_id SERIAL PRIMARY KEY,
    name    VARCHAR(50) NOT NULL
);

CREATE TABLE parking_slot (
    slot_id   SERIAL PRIMARY KEY,
    zone_id   INT NOT NULL REFERENCES zone(zone_id),
    slot_code VARCHAR(10) UNIQUE NOT NULL, -- e.g. A01
    status    VARCHAR(20) NOT NULL DEFAULT 'Available'
              CHECK (status IN ('Available', 'Reserved', 'Occupied'))
);

CREATE TABLE reservation (
    reservation_id SERIAL PRIMARY KEY,
    customer_id    INT NOT NULL REFERENCES customer(customer_id),
    vehicle_id     INT NOT NULL REFERENCES vehicle(vehicle_id),
    slot_id        INT NOT NULL REFERENCES parking_slot(slot_id),
    reserve_date   DATE NOT NULL,
    start_time     TIME NOT NULL,
    end_time       TIME NOT NULL,
    status         VARCHAR(20) NOT NULL DEFAULT 'Pending'
                   CHECK (status IN ('Pending', 'Confirmed', 'Cancelled'))
    -- TODO: أضف CONSTRAINT لمنع تعارض الحجوزات على نفس الـslot لو حبيتوا تطبقوه على مستوى الـDB
);

CREATE TABLE parking_session (
    session_id     SERIAL PRIMARY KEY,
    reservation_id INT NOT NULL REFERENCES reservation(reservation_id),
    check_in_time  TIMESTAMP,
    check_out_time TIMESTAMP,
    is_active      BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE payment (
    payment_id SERIAL PRIMARY KEY,
    session_id INT NOT NULL REFERENCES parking_session(session_id),
    amount     NUMERIC(10, 2) NOT NULL,
    status     VARCHAR(20) NOT NULL DEFAULT 'Pending'
               CHECK (status IN ('Pending', 'Paid')),
    paid_at    TIMESTAMP
);

-- TODO: أضيفوا Indexes على الأعمدة اللي هتتفلتر/تتبحث كتير
-- مثال: CREATE INDEX idx_reservation_slot ON reservation(slot_id);
