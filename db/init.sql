CREATE ROLE "user" WITH LOGIN PASSWORD 'password';
GRANT ALL PRIVILEGES ON DATABASE "ros_db" TO "user";

CREATE TABLE IF NOT EXISTS spatial_points (
    x INTEGER NOT NULL,
    y INTEGER NOT NULL,
    z INTEGER NOT NULL,
    color_r INTEGER NOT NULL,
    color_g INTEGER NOT NULL,
    color_b INTEGER NOT NULL,
    color_a INTEGER NOT NULL,
    timestamp BIGINT NOT NULL,
    nb_records INTEGER,
    PRIMARY KEY (x, y, z)
);
CREATE TABLE IF NOT EXISTS scans_binary (
    drone_id SMALLINT NOT NULL,
    seq INTEGER,
    flags SMALLINT,
    data BYTEA NOT NULL,
    created_at TIMESTAMP DEFAULT now()
);

ALTER TABLE spatial_points OWNER TO "user";
ALTER TABLE scans_binary OWNER TO "user";