package main

import (
	"encoding/hex"
	"encoding/json"
	"log"
	"math"
	"math/bits"
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

func handleQuality(store *Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		values, err := store.AllValues()
		if err != nil {
			log.Printf("quality error: %v", err)
			http.Error(w, "internal error", http.StatusInternalServerError)
			return
		}

		if len(values) == 0 {
			w.Header().Set("Content-Type", "application/json")
			json.NewEncoder(w).Encode(map[string]interface{}{
				"samples": 0,
			})
			return
		}

		// Concatenate all entropy values
		var all []byte
		for _, v := range values {
			all = append(all, v...)
		}

		totalBits := len(all) * 8
		onesCount := 0
		for _, b := range all {
			onesCount += bits.OnesCount8(b)
		}

		// Byte frequency histogram
		var freq [256]int
		for _, b := range all {
			freq[b]++
		}

		// Chi-squared test on byte distribution
		expected := float64(len(all)) / 256.0
		chiSq := 0.0
		for i := 0; i < 256; i++ {
			diff := float64(freq[i]) - expected
			chiSq += (diff * diff) / expected
		}

		// p-value approximation for chi-squared with 255 degrees of freedom
		// Using Wilson-Hilferty normal approximation
		df := 255.0
		z := math.Pow(chiSq/df, 1.0/3.0) - (1.0 - 2.0/(9.0*df))
		z /= math.Sqrt(2.0 / (9.0 * df))
		pValue := 0.5 * math.Erfc(z/math.Sqrt2)

		// Serial correlation coefficient (lag 1)
		n := len(all)
		var sumXY, sumX, sumY, sumX2, sumY2 float64
		for i := 0; i < n-1; i++ {
			x := float64(all[i])
			y := float64(all[i+1])
			sumXY += x * y
			sumX += x
			sumY += y
			sumX2 += x * x
			sumY2 += y * y
		}
		nf := float64(n - 1)
		corr := (nf*sumXY - sumX*sumY) / math.Sqrt((nf*sumX2-sumX*sumX)*(nf*sumY2-sumY*sumY))
		if math.IsNaN(corr) {
			corr = 0
		}

		// Raw bytes as hex for bitmap rendering
		rawHex := hex.EncodeToString(all)

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"samples":            len(values),
			"total_bits":         totalBits,
			"ones_count":         onesCount,
			"ones_pct":           float64(onesCount) / float64(totalBits) * 100,
			"byte_freq":          freq,
			"chi_squared":        math.Round(chiSq*100) / 100,
			"chi_squared_pvalue": math.Round(pValue*10000) / 10000,
			"serial_correlation": math.Round(corr*10000) / 10000,
			"raw":                rawHex,
		})
	}
}

func handleReset(store *Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		if err := store.Reset(); err != nil {
			log.Printf("reset error: %v", err)
			http.Error(w, "internal error", http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
	}
}
