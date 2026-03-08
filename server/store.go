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

	return &Store{db: db}, nil
}

func (s *Store) Close() error {
	return s.db.Close()
}

func (s *Store) Insert(value, signature []byte) error {
	_, err := s.db.Exec(
		"INSERT INTO entropy (value, signature) VALUES (?, ?)",
		value, signature,
	)
	return err
}

func (s *Store) Dispense() (*EntropyRow, error) {
	// UPDATE + RETURNING in a single statement — no race between SELECT and UPDATE
	row := s.db.QueryRow(
		`UPDATE entropy SET dispensed = 1, dispensed_at = datetime('now')
		 WHERE id = (SELECT id FROM entropy WHERE dispensed = 0 ORDER BY id ASC LIMIT 1)
		 RETURNING id, value, created_at`,
	)

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

func (s *Store) Status() (*Status, error) {
	var st Status

	err := s.db.QueryRow("SELECT COUNT(*) FROM entropy WHERE dispensed = 0").Scan(&st.Available)
	if err != nil {
		return nil, err
	}

	err = s.db.QueryRow("SELECT COUNT(*) FROM entropy").Scan(&st.TotalReceived)
	if err != nil {
		return nil, err
	}

	err = s.db.QueryRow("SELECT COUNT(*) FROM entropy WHERE dispensed = 1").Scan(&st.TotalDispensed)
	if err != nil {
		return nil, err
	}

	var lastReceived sql.NullString
	err = s.db.QueryRow("SELECT MAX(created_at) FROM entropy").Scan(&lastReceived)
	if err != nil {
		return nil, err
	}
	if lastReceived.Valid {
		st.LastReceived = lastReceived.String
	}

	return &st, nil
}

func (s *Store) AllValues() ([][]byte, error) {
	return s.ValuesRange(0, 0)
}

func (s *Store) ValuesRange(fromID, toID int) ([][]byte, error) {
	var rows *sql.Rows
	var err error
	if fromID > 0 && toID > 0 {
		rows, err = s.db.Query("SELECT value FROM entropy WHERE id >= ? AND id <= ? ORDER BY id ASC", fromID, toID)
	} else if fromID > 0 {
		rows, err = s.db.Query("SELECT value FROM entropy WHERE id >= ? ORDER BY id ASC", fromID)
	} else if toID > 0 {
		rows, err = s.db.Query("SELECT value FROM entropy WHERE id <= ? ORDER BY id ASC", toID)
	} else {
		rows, err = s.db.Query("SELECT value FROM entropy ORDER BY id ASC")
	}
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

func (s *Store) IDRange() (minID, maxID int, err error) {
	err = s.db.QueryRow("SELECT COALESCE(MIN(id),0), COALESCE(MAX(id),0) FROM entropy").Scan(&minID, &maxID)
	return
}

func (s *Store) TimeRangeForIDs(fromID, toID int) (firstTime, lastTime string, err error) {
	query := "SELECT COALESCE(MIN(created_at),''), COALESCE(MAX(created_at),'') FROM entropy WHERE 1=1"
	var args []interface{}
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

func (s *Store) Reset() error {
	_, err := s.db.Exec("DELETE FROM entropy")
	return err
}
