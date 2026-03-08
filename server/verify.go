package main

import (
	"crypto/ed25519"
	"crypto/sha256"
	"fmt"
)

type BeaconKey struct {
	Name      string
	PublicKey ed25519.PublicKey
	ID        string // first 8 hex chars of SHA-256 of public key
}

var beaconKeys []BeaconKey

func init() {
	beaconKeys = []BeaconKey{
		newBeaconKey("beacon-1", ed25519.PublicKey{
			0x79, 0xb5, 0x56, 0x2e, 0x8f, 0xe6, 0x54, 0xf9,
			0x40, 0x78, 0xb1, 0x12, 0xe8, 0xa9, 0x8b, 0xa7,
			0x90, 0x1f, 0x85, 0x3a, 0xe6, 0x95, 0xbe, 0xd7,
			0xe0, 0xe3, 0x91, 0x0b, 0xad, 0x04, 0x96, 0x64,
		}),
	}
}

func newBeaconKey(name string, pub ed25519.PublicKey) BeaconKey {
	h := sha256.Sum256(pub)
	return BeaconKey{
		Name:      name,
		PublicKey: pub,
		ID:        fmt.Sprintf("%x", h[:4]),
	}
}

// verifySignature tries each known beacon key. Returns (beaconID, true) on
// success or ("", false) if no key verified the signature.
func verifySignature(entropy, signature []byte) (string, bool) {
	for _, bk := range beaconKeys {
		if ed25519.Verify(bk.PublicKey, entropy, signature) {
			return bk.ID, true
		}
	}
	return "", false
}

// beaconKeyByID returns the BeaconKey for a given short ID, or nil.
func beaconKeyByID(id string) *BeaconKey {
	for i := range beaconKeys {
		if beaconKeys[i].ID == id {
			return &beaconKeys[i]
		}
	}
	return nil
}
