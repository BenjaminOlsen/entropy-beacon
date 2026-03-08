package main

import (
	"flag"
	"log"
	"net/http"
)

func main() {
	addr := flag.String("addr", ":8080", "listen address")
	dbPath := flag.String("db", "entropy.db", "SQLite database path")
	flag.Parse()

	store, err := NewStore(*dbPath)
	if err != nil {
		log.Fatalf("failed to open database: %v", err)
	}
	defer store.Close()

	mux := http.NewServeMux()
	mux.HandleFunc("/api/ingest", handleIngest(store))
	mux.HandleFunc("/api/entropy", handleEntropy(store))
	mux.HandleFunc("/api/status", handleStatus(store))
	mux.HandleFunc("/api/quality", handleQuality(store))
	mux.HandleFunc("/api/quality/raw", handleQualityRaw(store))
	mux.HandleFunc("/api/reset", handleReset(store))
	mux.Handle("/", http.FileServer(http.Dir("static")))

	log.Printf("listening on %s", *addr)
	log.Fatal(http.ListenAndServe(*addr, mux))
}
