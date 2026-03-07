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
	tx, err := s.db.Begin()
	if err != nil {
		return nil, err
	}
	defer tx.Rollback()

	row := tx.QueryRow(
		"SELECT id, value, created_at FROM entropy WHERE dispensed = 0 ORDER BY id ASC LIMIT 1",
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

	_, err = tx.Exec(
		"UPDATE entropy SET dispensed = 1, dispensed_at = datetime('now') WHERE id = ?",
		e.ID,
	)
	if err != nil {
		return nil, err
	}

	return &e, tx.Commit()
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
	rows, err := s.db.Query("SELECT value FROM entropy ORDER BY id ASC")
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

func (s *Store) Reset() error {
	_, err := s.db.Exec("DELETE FROM entropy")
	return err
}
