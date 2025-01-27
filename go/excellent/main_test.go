package main

import "testing"

func TestEvenOrOdd(t *testing.T) {
	tests := []struct {
		name string
		n    int
		want string
	}{
		{"Even", 2, "Even"},
		{"Odd", 3, "Odd"},
		{"Even", 4, "Even"},
		{"Odd", 5, "Odd"},
		{"Even", 6, "Even"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := EvenOrOdd(tt.n); got != tt.want {
				t.Errorf("EvenOrOdd() = %v, want %v", got, tt.want)
			}
		})
	}
}