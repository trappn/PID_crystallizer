$fn=50;
use <scad_libs//cyl_head_bolt.scad>

module fillet(r, h) {
// generates rounded corners
    translate([r / 2, r / 2, 0])
        difference() {
            cube([r + 0.01, r + 0.01, h], center = true);
            translate([r/2, r/2, 0])
                cylinder(r = r, h = h + 1, center = true);
        }
}

module dummy() {
color("silver") {
  translate([0,8.5,0])  cube([76,45,4.5]); 
  difference() {
    translate([0,0,4.5])  cube([76,62,2]); 
    translate([22,5,-1])  cylinder(h=10, d=3.5);
    translate([54,5,-1])  cylinder(h=10, d=3.5);
    translate([22,57,-1])  cylinder(h=10, d=3.5);
    translate([54,57,-1])  cylinder(h=10, d=3.5);
  }    
  translate([0,24,2.5])  rotate([0,90,180])  cylinder(h=12, d=3.8);
  translate([0,39,2.5])  rotate([0,90,180])  cylinder(h=12, d=3.8);
}
}

module positive() {
  difference() {
    translate([-22,-19,-23])  cube([120,100,80]);
    // rounded corners:
    translate([-22,-19,20])  rotate([0,0,0])  fillet(20,100);
    translate([-22,81,20])  rotate([0,0,270])  fillet(20,100);
    translate([98,-19,20])  rotate([0,0,90])  fillet(20,100);
    translate([98,81,20])  rotate([0,0,180])  fillet(20,100);
    // cable hole:
    translate([-22.1,21.4,-5.5])  cube([40,20.2,10.2]);  
    // lower recess:
    translate([0,8,-8])  cube([78,46,25]);
    // upper recess: 
    hull() {
      translate([0,-0.5,4.5])  cube([78,63,1]); 
      translate([-2,-2.5,57])  cube([82,67,1]); 
    }
  }
}

module mold_inner(d=1) {
  difference() {
    translate([0,8,-8])  cube([78,46,25]);
    // socket for teflon spacer 40*20*10:
    translate([-22,21.4,-5.5])  cube([40,20.2,10.2]);
  }
    translate([-2,-2.5,57])  cube([82,67,5]);
    translate([-24-d,21.5,60])  cube([124+2*d,20,2]);
    translate([28,-21-d,60])  cube([20,104+2*d,2]);
    // spacers:
    translate([-22-d,21.5,57])  cube([d,20,3]);
    translate([98,21.5,57])  cube([d,20,3]);
    translate([28,-19-d,57])  cube([20,d,3]);
    translate([28,81,57])  cube([20,d,3]);
    //downward clips:
    translate([-24-d,21.5,52])  cube([2,20,8]);
    translate([98+d,21.5,52])  cube([2,20,8]);
    translate([28,-21-d,52])  cube([20,2,8]);
    translate([28,81+d,52])  cube([20,2,8]);
    hull() {
      translate([0,-0.5,4.5])  cube([78,63,1]); 
      translate([-2,-2.5,57])  cube([82,67,1]);
    }
}

module mold_outer(d=1) {
color("blue",0.3) {
  difference() {
    translate([-22-d,-19-d,-23-d])  cube([120+2*d,100+2*d,80+d]); 
    difference() {
      translate([-22,-19,-23])  cube([120,100,81]);
      translate([-22,-19,20])  rotate([0,0,0])  fillet(20,100);
      translate([-22,81,20])  rotate([0,0,270])  fillet(20,100);
      translate([98,-19,20])  rotate([0,0,90])  fillet(20,100);
      translate([98,81,20])  rotate([0,0,180])  fillet(20,100); 
    }
    translate([-22-d,-19-d,20])  rotate([0,0,0])  fillet(20,100);
    translate([-22-d,81+d,20])  rotate([0,0,270])  fillet(20,100);
    translate([98+d,-19-d,20])  rotate([0,0,90])  fillet(20,100);
    translate([98+d,81+d,20])  rotate([0,0,180])  fillet(20,100); 
  }
}
}

module strain_relief1() {
color("red") {
  difference() {
    union() {
      translate([-30,15.5,-5.4])  cube([7.5,32,5]);  
      hull() {
        translate([-22,21.5,-5.4])  cube([13,20,5]);
        difference() {
          translate([-48,31.5,-0.4])  rotate([0,90,0])  cylinder(d=10,h=1);
          translate([-50,21.5,-0.4])  cube([5,20,10]);
        }
      }
    }    
    translate([-13,31.5,-30])  cylinder(h=100,d=3.5);  
    translate([-13,31.5,0.2])  nutcatch_parallel("M3");
    translate([-26,19,-1.4])  nutcatch_parallel("M3");
    translate([-26,44,-1.4])  nutcatch_parallel("M3");
    translate([-26,19,-5])  cylinder(d=3.5,h=10);
    translate([-26,44,-5])  cylinder(d=3.5,h=10);
    translate([-30.1,15.4,-5])  rotate([0,0,0])  fillet(3,100);
    translate([-30.1,47.6,-5])  rotate([0,0,270])  fillet(3,100);
    translate([-46,26.5,-10])  cube([4,1,20]);
    translate([-46,35.5,-10])  cube([4,1,20]); 
    translate([-51,31.5,-0.4])  rotate([0,90,0])  cylinder(d=6.5,h=12);
    hull() {
      translate([-40,31.5,-0.4])  rotate([0,90,0])  cylinder(d=6.5,h=1);
      translate([-9,31.5,-0.4])  rotate([0,90,0])  scale([1,3.5,1])  cylinder(d=5,h=1);
    }
  }
}
}

module strain_relief2() {
color("red") {
  difference() {
    union() {
      translate([-30,15.5,-0.4])  cube([7.5,32,2]);  
      hull() {
        translate([-22,21.5,-0.4])  cube([4,20,4.5]);
        difference() {
          translate([-48,31.5,-0.4])  rotate([0,90,0])  cylinder(d=10,h=1);
          translate([-50,21.5,-10.4])  cube([5,20,10]);
        }
      }
    }    
    translate([-26,19,-1.4])  nutcatch_parallel("M3");
    translate([-26,44,-1.4])  nutcatch_parallel("M3");
    translate([-26,19,-5])  cylinder(d=3.5,h=10);
    translate([-26,44,-5])  cylinder(d=3.5,h=10);
    translate([-30.1,15.4,-5])  rotate([0,0,0])  fillet(3,100);
    translate([-30.1,47.6,-5])  rotate([0,0,270])  fillet(3,100);
    translate([-46,26.5,-10])  cube([4,1,20]);
    translate([-46,35.5,-10])  cube([4,1,20]); 
    translate([-51,31.5,-0.4])  rotate([0,90,0])  cylinder(d=6.5,h=12);
    hull() {
      translate([-40,31.5,-0.4])  rotate([0,90,0])  cylinder(d=6.5,h=1);
      translate([-9,31.5,-0.4])  rotate([0,90,0])  scale([1,3.5,1])  cylinder(d=5,h=1);
    }
  }
}
}

module drill_jig() {
color("blue") {
  difference() {
    translate([-23.5,21.7,-24])  cube([15,19.6,19.5]);     
    translate([-22,21,-23])  cube([14,22,18]);
    translate([-13,31.5,-30])  cylinder(h=10,d=3.5);       
  }
}
}

module vialholder() {
color("orange") {
  difference() {
    translate([-14,-33,-10])  cube([76,40,15]);
    hull() {
      translate([0,0,0])  rotate([60,0,0])  cylinder(d=19,h=45);
      translate([0,0,30])  rotate([60,0,0])  cylinder(d=19,h=45);
    }
    hull() {
      translate([24,0,0])  rotate([60,0,0])  cylinder(d=19,h=45);
      translate([24,0,30])  rotate([60,0,0])  cylinder(d=19,h=45);
    }
    hull() {
      translate([48,0,0])  rotate([60,0,0])  cylinder(d=19,h=45);
      translate([48,0,30])  rotate([60,0,0])  cylinder(d=19,h=45);
    }
  }
}
}

module lidpositive() {
color("lightgrey") {
  difference() {
    translate([-1,-1.5,57.1])  cube([80,65,3]);
    translate([-1.1,-1.6,56])  rotate([0,0,0])  fillet(5,10);
    translate([-1.1,63.6,56])  rotate([0,0,270])  fillet(5,10);
    translate([79.1,-1.6,56])  rotate([0,0,90])  fillet(5,10);
    translate([79.1,63.6,56])  rotate([0,0,180])  fillet(5,10); 
  }
  difference() {
    translate([-22,-19,60])  cube([120,100,20]);
    // rounded corners:
    translate([-22,-19,70])  rotate([0,0,0])  fillet(20,50);
    translate([-22,81,70])  rotate([0,0,270])  fillet(20,50);
    translate([98,-19,70])  rotate([0,0,90])  fillet(20,50);
    translate([98,81,70])  rotate([0,0,180])  fillet(20,50);
  }
}
}

module lidmold(d=1) {
color("lightgrey") {
difference() {
  translate([-22-d,-19-d,57.1-d])  cube([120+2*d,100+2*d,27+d]);
  difference() {
    translate([-1,-1.5,57.1])  cube([80,65,3]);
    translate([-1.1,-1.6,56])  rotate([0,0,0])  fillet(5,10);
    translate([-1.1,63.6,56])  rotate([0,0,270])  fillet(5,10);
    translate([79.1,-1.6,56])  rotate([0,0,90])  fillet(5,10);
    translate([79.1,63.6,56])  rotate([0,0,180])  fillet(5,10); 
  }
  difference() {
    translate([-22,-19,60])  cube([120,100,25]);
    // rounded corners:
    translate([-22,-19,70])  rotate([0,0,0])  fillet(20,50);
    translate([-22,81,70])  rotate([0,0,270])  fillet(20,50);
    translate([98,-19,70])  rotate([0,0,90])  fillet(20,50);
    translate([98,81,70])  rotate([0,0,180])  fillet(20,50);
  }
}
}
}

module carrier() {
  difference() {
    translate([0,0,0])  cube([70,60,26]);
    translate([-1,10,3])  cube([72,40,26]);
    translate([-0.1,-0.1,10])  rotate([0,0,0])  fillet(5,60);
    translate([-0.1,60.1,10])  rotate([0,0,270])  fillet(5,60);
    translate([70.1,-0.1,10])  rotate([0,0,90])  fillet(5,60);
    translate([70.1,60.1,10])  rotate([0,0,180])  fillet(5,60);
    translate([75,30,-1])  cylinder(d=20,h=50);
    translate([-5,41,-1])  cylinder(d=20,h=50);
    translate([-5,18.5,-1])  cylinder(d=20,h=50);
    for (x = [0:3])
       for (y = [0:2])
          translate([15*x+8.5,22.5*y+7.5,-1])  cylinder(d=12,h=50);
    for (x = [0:3])
       for (y = [0:1])
          translate([15*x+16,22.5*y+18.5,-1])  cylinder(d=12,h=50);    
     
  }
}

carrier();
//dummy();
//positive();
//mold_inner(1);
//mold_outer(1); 
//strain_relief1();
//strain_relief2();
//translate([-20,0,0])  drill_jig();
//translate([14,50,23])  vialholder();
//lidpositive();
//lidmold(); 