$fn=80;
difference() {
  union() {
    cylinder(h=1,d=30);
    translate([0,0,1]) cylinder(h=6,d=28.2);
    translate([0,0,7]) cylinder(h=1,d1=28.2,d2=27);
  }
  translate([0,0,6])  cube([30,10,10],center=true);
}