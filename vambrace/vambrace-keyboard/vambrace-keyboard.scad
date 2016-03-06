/* Copyright 2016 John C G Sturdy --- will pick an open licence sometime
   Time-stamp: <2016-03-06 16:55:40 jcgs>

   Physical design pieces for a vambrace touch keyboard
 */

rivet_slot_length = 3;
rivet_slot_width = 0.5;
rivet_size = 7;

key_spacing = 11;

module key_row(n) {
     for (i = [0 : n - 1]) {
	  translate([0, i * key_spacing]) rivet();
     }
}

module keyboard_core(ns) {
     for (i = [0 : len(ns)-1]) {
	  translate([key_spacing * i, key_spacing * i / 3]) key_row(ns[i]);
     }
}

module rivet() {
     offset = (rivet_size - rivet_slot_length) / 2;
     translate([0, offset]) square([rivet_slot_width, rivet_slot_length]);
     translate([offset, 0]) square([rivet_slot_length, rivet_slot_width]);
     translate([rivet_size, offset]) square([rivet_slot_width, rivet_slot_length]);
     translate([offset, rivet_size]) square([rivet_slot_length, rivet_slot_width]);
}
 
keyboard_core([12,12,13,10]);
