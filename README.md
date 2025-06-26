# server_udp

Serveur UDP multithreadé écrit en C++ (ASIO + libpq) pour réception, réassemblage, parsing et insertion de cartes 3D OctoMap dans une base PostgreSQL.

## Fonctionnalités

- Réception et réassemblage de blobs OctoMap fragmentés via UDP.
- Décompression zlib niveau 1.
- Extraction des voxels `(x, y, z, r, g, b, a, timestamp)`.
- Insertion dans la base PostgreSQL via `INSERT ... ON CONFLICT ... UPDATE`.

## Lancement de la stack

Créer le réseau virtuel :

```bash
docker network create mapnet
```

Démarrer le serveur :

```bash
docker compose up --build
```

## Variables d’environnement

- `DB_HOST` : hôte de la base de données (ex : `db`)
- `DB_PORT` : port PostgreSQL (ex : `5432`)
- `DB_USER` : nom d’utilisateur PostgreSQL
- `DB_PASSWORD` : mot de passe PostgreSQL
- `DB_NAME` : nom de la base (ex : `ros_db`)
- `UDP_PORT` : port d’écoute UDP (par défaut : `9000`)

## Base de données

Schéma de la table `spatial_points` :

```sql
CREATE TABLE spatial_points (
  x INT, y INT, z INT,
  color_r SMALLINT, color_g SMALLINT, color_b SMALLINT, color_a SMALLINT,
  timestamp BIGINT,
  nb_records INT,
  PRIMARY KEY (x, y, z)
);

CREATE INDEX sp_xyz_gist ON spatial_points USING gist (point(x, y, z));
```

## Vérification de l’insertion

```bash
docker exec -it <nom_container_db> psql -U postgres -d ros_db \
  -c "SELECT COUNT(*) FROM spatial_points;"
```

## Arborescence

```
.
├── src/                  # Code source du serveur
│   ├── main.cpp
│   └── ...
├── db/init.sql          # Script d'initialisation
├── Dockerfile           # Build du serveur
├── docker-compose.yml   # Stack serveur + DB
├── include/             # Headers
```

## Prérequis système (build hors Docker)

- `libasio-dev`
- `libpq-dev`
- `zlib1g-dev`
- `cmake`, `g++`

## Documentation associée

- Dossier d’architecture technique : DAT Final (pdf)
- Format de l’entête UDP et du message binaire (cf. annexes DAT)
