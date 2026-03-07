package main

import (
	"encoding/hex"
	"encoding/json"
	"log"
	"net/http"
	"strings"
)

type ingestRequest struct {
	Entropy   string `json:"entropy"`
	Signature string `json:"signature"`
}

func handleIngest(store *Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var req ingestRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "bad request", http.StatusBadRequest)
			return
		}

		entropy, err := hex.DecodeString(strings.TrimSpace(req.Entropy))
		if err != nil || len(entropy) != 32 {
			http.Error(w, "entropy must be 64 hex chars (32 bytes)", http.StatusBadRequest)
			return
		}

		sig, err := hex.DecodeString(strings.TrimSpace(req.Signature))
		if err != nil || len(sig) != 64 {
			http.Error(w, "signature must be 128 hex chars (64 bytes)", http.StatusBadRequest)
			return
		}

		if !verifySignature(entropy, sig) {
			http.Error(w, "signature verification failed", http.StatusUnauthorized)
			return
		}

		if err := store.Insert(entropy, sig); err != nil {
			if strings.Contains(err.Error(), "UNIQUE constraint") {
				http.Error(w, "duplicate entropy value", http.StatusConflict)
				return
			}
			log.Printf("insert error: %v", err)
			http.Error(w, "internal error", http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
	}
}

func handleEntropy(store *Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		row, err := store.Dispense()
		if err != nil {
			log.Printf("dispense error: %v", err)
			http.Error(w, "internal error", http.StatusInternalServerError)
			return
		}

		if row == nil {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"entropy":    hex.EncodeToString(row.Value),
			"id":         row.ID,
			"created_at": row.CreatedAt.Format("2006-01-02T15:04:05Z"),
		})
	}
}

func handleStatus(store *Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		st, err := store.Status()
		if err != nil {
			log.Printf("status error: %v", err)
			http.Error(w, "internal error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(st)
	}
}
