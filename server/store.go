package main

import (
	"database/sql"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

type Store struct {
	db *sql.DB
}

type EntropyRow struct {
	ID        int64
	Value     []byte
	Signature []byte
	CreatedAt time.Time
}

type Status struct {
	Available      int    `json:"available"`
	TotalReceived  int    `json:"total_received"`
	TotalDispensed int    `json:"total_dispensed"`
	LastReceived   string `json:"last_received"`
}

type BeaconInfo struct {
	ID           string `json:"id"`
	Name         string `json:"name"`
	Samples      int    `json:"samples"`
	LastReceived string `json:"last_received"`
}

func NewStore(path string) (*Store, error) {
	db, err := sql.Open("sqlite3", path+"?_journal_mode=WAL")
	if err != nil {
		return nil, err
	}

	_, err = db.Exec(`CREATE TABLE IF NOT EXISTS entropy (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		value BLOB NOT NULL UNIQUE,
		signature BLOB NOT NULL,
		dispensed INTEGER NOT NULL DEFAULT 0,
		created_at TEXT NOT NULL DEFAULT (datetime('now')),
		dispensed_at TEXT
	)`)
	if err != nil {
		return nil, err
	}

	// Add beacon_id column if it doesn't exist
	if !columnExists(db, "entropy", "beacon_id") {
		_, err = db.Exec(`ALTER TABLE entropy ADD COLUMN beacon_id TEXT NOT NULL DEFAULT ''`)
		if err != nil {
			return nil, err
		}
	}

	// Backfill existing rows with the first known beacon's ID
	if len(beaconKeys) > 0 {
		_, err = db.Exec(`UPDATE entropy SET beacon_id = ? WHERE beacon_id = ''`, beaconKeys[0].ID)
		if err != nil {
			return nil, err
		}
	}

	return &Store{db: db}, nil
}

func columnExists(db *sql.DB, table, column string) bool {
	rows, err := db.Query("PRAGMA table_info(" + table + ")")
	if err != nil {
		return false
	}
	defer rows.Close()
	for rows.Next() {
		var cid int
		var name, typ string
		var notNull int
		var dflt sql.NullString
		var pk int
		if err := rows.Scan(&cid, &name, &typ, &notNull, &dflt, &pk); err != nil {
			return false
		}
		if name == column {
			return true
		}
	}
	return false
}

func (s *Store) Close() error {
	return s.db.Close()
}

func (s *Store) Insert(value, signature []byte, beaconID string) error {
	_, err := s.db.Exec(
		"INSERT INTO entropy (value, signature, beacon_id) VALUES (?, ?, ?)",
		value, signature, beaconID,
	)
	return err
}

func (s *Store) Dispense(beaconID string) (*EntropyRow, error) {
	var query string
	var args []interface{}
	if beaconID != "" {
		query = `UPDATE entropy SET dispensed = 1, dispensed_at = datetime('now')
		 WHERE id = (SELECT id FROM entropy WHERE dispensed = 0 AND beacon_id = ? ORDER BY id ASC LIMIT 1)
		 RETURNING id, value, created_at`
		args = append(args, beaconID)
	} else {
		query = `UPDATE entropy SET dispensed = 1, dispensed_at = datetime('now')
		 WHERE id = (SELECT id FROM entropy WHERE dispensed = 0 ORDER BY id ASC LIMIT 1)
		 RETURNING id, value, created_at`
	}

	row := s.db.QueryRow(query, args...)
	var e EntropyRow
	var createdStr string
	if err := row.Scan(&e.ID, &e.Value, &createdStr); err != nil {
		if err == sql.ErrNoRows {
			return nil, nil
		}
		return nil, err
	}

	e.CreatedAt, _ = time.Parse("2006-01-02 15:04:05", createdStr)
	return &e, nil
}

func (s *Store) Status(beaconID string) (*Status, error) {
	var st Status
	filter := ""
	var args []interface{}
	if beaconID != "" {
		filter = " AND beacon_id = ?"
		args = []interface{}{beaconID}
	}

	err := s.db.QueryRow("SELECT COUNT(*) FROM entropy WHERE dispensed = 0"+filter, args...).Scan(&st.Available)
	if err != nil {
		return nil, err
	}

	err = s.db.QueryRow("SELECT COUNT(*) FROM entropy WHERE 1=1"+filter, args...).Scan(&st.TotalReceived)
	if err != nil {
		return nil, err
	}

	err = s.db.QueryRow("SELECT COUNT(*) FROM entropy WHERE dispensed = 1"+filter, args...).Scan(&st.TotalDispensed)
	if err != nil {
		return nil, err
	}

	var lastReceived sql.NullString
	err = s.db.QueryRow("SELECT MAX(created_at) FROM entropy WHERE 1=1"+filter, args...).Scan(&lastReceived)
	if err != nil {
		return nil, err
	}
	if lastReceived.Valid {
		st.LastReceived = lastReceived.String
	}

	return &st, nil
}

func (s *Store) AllValues(beaconID string) ([][]byte, error) {
	return s.ValuesRange(beaconID, 0, 0)
}

func (s *Store) ValuesRange(beaconID string, fromID, toID int) ([][]byte, error) {
	query := "SELECT value FROM entropy WHERE 1=1"
	var args []interface{}
	if beaconID != "" {
		query += " AND beacon_id = ?"
		args = append(args, beaconID)
	}
	if fromID > 0 {
		query += " AND id >= ?"
		args = append(args, fromID)
	}
	if toID > 0 {
		query += " AND id <= ?"
		args = append(args, toID)
	}
	query += " ORDER BY id ASC"

	rows, err := s.db.Query(query, args...)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var values [][]byte
	for rows.Next() {
		var v []byte
		if err := rows.Scan(&v); err != nil {
			return nil, err
		}
		values = append(values, v)
	}
	return values, rows.Err()
}

func (s *Store) IDRange(beaconID string) (minID, maxID int, err error) {
	query := "SELECT COALESCE(MIN(id),0), COALESCE(MAX(id),0) FROM entropy WHERE 1=1"
	var args []interface{}
	if beaconID != "" {
		query += " AND beacon_id = ?"
		args = append(args, beaconID)
	}
	err = s.db.QueryRow(query, args...).Scan(&minID, &maxID)
	return
}

func (s *Store) TimeRangeForIDs(beaconID string, fromID, toID int) (firstTime, lastTime string, err error) {
	query := "SELECT COALESCE(MIN(created_at),''), COALESCE(MAX(created_at),'') FROM entropy WHERE 1=1"
	var args []interface{}
	if beaconID != "" {
		query += " AND beacon_id = ?"
		args = append(args, beaconID)
	}
	if fromID > 0 {
		query += " AND id >= ?"
		args = append(args, fromID)
	}
	if toID > 0 {
		query += " AND id <= ?"
		args = append(args, toID)
	}
	err = s.db.QueryRow(query, args...).Scan(&firstTime, &lastTime)
	return
}

func (s *Store) ListBeacons() ([]BeaconInfo, error) {
	rows, err := s.db.Query(`SELECT beacon_id, COUNT(*), COALESCE(MAX(created_at),'') FROM entropy GROUP BY beacon_id`)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var beacons []BeaconInfo
	for rows.Next() {
		var b BeaconInfo
		if err := rows.Scan(&b.ID, &b.Samples, &b.LastReceived); err != nil {
			return nil, err
		}
		if bk := beaconKeyByID(b.ID); bk != nil {
			b.Name = bk.Name
		}
		beacons = append(beacons, b)
	}
	return beacons, rows.Err()
}

func (s *Store) Reset() error {
	_, err := s.db.Exec("DELETE FROM entropy")
	return err
}
