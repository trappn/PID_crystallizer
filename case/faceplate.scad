$fn=60;

module blank() {
difference() {
  cube([81.5,61,4]);
  // window:
  translate([5.6,23,-0.5])  cube([71,24.8,5]);
  // buttons:
  translate([16.7,9.3,-0.5])  cylinder(h=5,d=6);
  translate([24.9,13.2,-0.5])  cylinder(h=5,d=6); 
  translate([24.9,5.4,-0.5])  cylinder(h=5,d=6); 
  translate([32.9,9.3,-0.5])  cylinder(h=5,d=6); 
  translate([40.7,9.3,-0.5])  cylinder(h=5,d=6);
  // M3 screws:
  translate([3,19.75,-0.5])  cylinder(h=5,d=3.5); 
  translate([3,50.75,-0.5])  cylinder(h=5,d=3.5); 
  translate([78.5,19.75,-0.5])  cylinder(h=5,d=3.5); 
  translate([78.5,50.75,-0.5])  cylinder(h=5,d=3.5);    
}
}

module button() {
color("green") {
difference() {
  union() {
    cylinder(h=7.5,d=5.6);
    cylinder(h=1,d=6.6);  
  }
 translate([0,0,-1])  cylinder(h=6,d=3.6);
 translate([0,0,12])  sphere(d=10);
}
}
}

module button_short() {
color("red") {
difference() {
  union() {
    cylinder(h=6.5,d=5.6);
    cylinder(h=1,d=6.6);  
  }
 translate([0,0,-1])  cylinder(h=6,d=3.6);
 translate([0,0,11])  sphere(d=10);
}
}
}

module front() {
// main (fits https://www.thingiverse.com/thing:2321751) 
translate([-8.85,-90.9,0])  {
difference() {
  cube([98.8,161.6,4]);
  // corners:
  translate([-3.5,-20,-1])  rotate([0,0,45])  cube([20,20,6]);
  translate([102.2,-20,-1])  rotate([0,0,45])  cube([20,20,6]);
  translate([-3.5,153.2,-1])  rotate([0,0,45])  cube([20,20,6]);
  translate([102.2,153.2,-1])  rotate([0,0,45])  cube([20,20,6]);
  // M3 through holes:
  translate([0.9,30.9,-0.5])  cylinder(h=10,d=3.4);
  translate([97.9,30.9,-0.5])  cylinder(h=10,d=3.4);
  translate([0.9,131.9,-0.5])  cylinder(h=10,d=3.4);
  translate([97.9,131.9,-0.5])  cylinder(h=10,d=3.4);
  // sink holes:
  translate([0.9,30.9,1])  cylinder(h=10,d=5.7);
  translate([97.9,30.9,1])  cylinder(h=10,d=5.7);
  translate([0.9,131.9,1])  cylinder(h=10,d=5.7);
  translate([97.9,131.9,1])  cylinder(h=10,d=5.7);
  // Thermistor sockets:
  translate([35,80,-1])  cylinder(h=10,d=6.5);
  translate([35,65,-1])  cylinder(h=10,d=6.5);
  // Heater power sockets:
  translate([20,80,-1])  cylinder(h=10,d=8.5);
  translate([20,65,-1])  cylinder(h=10,d=8.5);
  // SSR LED window:
  translate([77.9,32.9,-0.1])  cylinder(h=4.2,d1=3,d2=6);
  // SSR fixings + sink holes (M3):
  translate([65.9,17.9,-1])  cylinder(h=10,d=3.5);
  translate([65.9,65.9,-1])  cylinder(h=10,d=3.5);
  translate([65.9,17.9,1])  cylinder(h=10,d=5.7);
  translate([65.9,65.9,1])  cylinder(h=10,d=5.7);
  // USB socket + sink holes (M3):
  //translate([71.6,80,0]) {
  //rotate([0,0,90]) {
  //  translate([0,0,0])  cube([9,10,10],center=true);
  //  translate([0,0,4])  cube([15,16,4],center=true);
  //  translate([0,15,-1])  cylinder(d=3.5,h=10);  
  //  translate([0,-15,-1])  cylinder(d=3.5,h=10);
  //  translate([0,15,2])  cylinder(d=5.7,h=10);  
  //  translate([0,-15,2])  cylinder(d=5.7,h=10);
  //}
  //}
  // TEXT:
  // shift:
  translate([57.5,99.6,3])  linear_extrude(height=2)  text("", font = "Wingdings3", size=6,halign="center",valign="center");
  // back:
  translate([18,100.2,3])  linear_extrude(height=2)  text("", font = "Wingdings3", size=4,halign="center",valign="center");
  // + and -
  translate([33.75,110,3])  linear_extrude(height=2)  text("+", font = "Liberation Sans:style=Bold", size=6,halign="center",valign="center");
  translate([33.75,90,3])  linear_extrude(height=2)  text("-", font = "Liberation Sans:style=Bold", size=6,halign="center",valign="center");
  // heater active symbol:
  translate([79.2,24,3])  rotate([0,0,90])  linear_extrude(height=2)  text("≈", size=12,halign="center",valign="center");
  // DISPLAY:
  translate([8.85,90.9,0])  {
    // display window:
    translate([5.6,23,-0.5])  cube([71,24.8,5]);
    // display buttons:
    translate([16.7,9.3,-0.5])  cylinder(h=5,d=6);
    translate([24.9,13.2,-0.5])  cylinder(h=5,d=6); 
    translate([24.9,5.4,-0.5])  cylinder(h=5,d=6); 
    translate([32.9,9.3,-0.5])  cylinder(h=5,d=6); 
    translate([40.7,9.3,-0.5])  cylinder(h=5,d=6);
    // display M3 screws:
    translate([3,19.75,-0.5])  cylinder(h=5,d=3.5); 
    translate([3,50.75,-0.5])  cylinder(h=5,d=3.5); 
    translate([78.5,19.75,-0.5])  cylinder(h=5,d=3.5); 
    translate([78.5,50.75,-0.5])  cylinder(h=5,d=3.5);
    // display M3 sink holes:
    translate([3,19.75,1])  cylinder(h=5,d=5.7); 
    translate([3,50.75,1])  cylinder(h=5,d=5.7); 
    translate([78.5,19.75,1])  cylinder(h=5,d=5.7); 
    translate([78.5,50.75,1])  cylinder(h=5,d=5.7);
  }   
}
}
}

// SSR:
//translate([35,-80,-24])  cube([44,60,25]);

//color("red",0.3) translate([0,0,1]) blank();

//translate([16.7,9.3,-0.5])  button();
//translate([24.9,13.2,-0.5])  button();
//translate([24.9,5.4,-0.5])  button();
//translate([32.9,9.3,-0.5])  button();
//translate([40.7,9.3,-0.5])  button();
front();
//button();
//translate([0,10,0])  button_short();

