
r=7;
t=2;
d=32;
centerButtonToBottom=94;

pokerR=4;

$fn=32;

  caseH=centerButtonToBottom+pokerR;

// case();
motorStabilzier();


// translate([2.2,-0.1,2.2]) cube([48,40,14]);
// poker();
// translate([25,0,0]) poker();


// translate([0,pokerR,r+2.8]) difference() {
//   color("blue") translate([-24,-6,0]) cube([48,40,14]);
//   presserShape();
// }


module motorStabilzier() {
 cube([23-0.5,23-.5,1.5]);
}

module fullDrawer() {
translate([0,pokerR,r+2.8]) difference() {
   translate([-24,-6,0]) drawer();
  presserShape();
 }
}
module drawer() {
  difference() {
    cube([48,centerButtonToBottom+pokerR+2,14]);
    translate([2,40,-1]) cube([44,centerButtonToBottom+pokerR-40,16]);
    translate([24,60,8]) rotate([-90,0,0]) cylinder(r=3,h=50);
  }
}

module case() {  
  difference() {
  translate([(48+4.4)/-2,0,r+1]) difference() {
    cube([48+4.4,caseH,14+3.4]);
    translate([2,-0.1,1.5]) cube([48.4,caseH+0.2,14.4]);
  }
      hull() {
        translate([-12.5,pokerR,-0.1]) cylinder(r=pokerR+0.5,h=115+0.2);
        translate([12.5,pokerR,-0.1]) cylinder(r=pokerR+0.5,h=115+0.2);
        translate([-34/2,0,-0.1]) cube([34,pokerR,115+0.2]);
      }
  }

  translate([(48+4.4)/-2,caseH,r+1]) 
  difference() {
    cube([48+4.4,2,14+3.4]);
    translate([(48+4.4)/2,3,(14+3.4)/2]) rotate([90,0,0]) cylinder(r=3,h=5);
  }
  caseBit();
}



module poker() {
  hull() torus(3,1);
  translate([0,0,14]) hull() torus(3,1);

  difference() {
    cylinder(r=pokerR,h=14);

    translate([0,0,7.5]) rotate([90,0,0]) {
      translate([0,0,-1]) hull() {
        translate([-10,0,0]) cylinder(r=4,h=2);
        translate([10,0,0]) cylinder(r=4,h=2);
      }
      translate([0,0,-10]) cylinder(r=0.5,h=20);
    }
  }
}


module caseBit() {
  caseH=centerButtonToBottom+pokerR;
  rotate([-90,0,0])   translate([0,0,caseH])      hull() {
        translate([-d/2,0,0]) cylinder(r=r+t,h=t);
        translate([d/2,0,0]) cylinder(r=r+t,h=t);
      }
  difference() {
    union() {
  rotate([-90,0,0]) {
    difference() {
      hull() {
        translate([-d/2,0,0]) cylinder(r=r+t,h=caseH);
        translate([d/2,0,0]) cylinder(r=r+t,h=caseH);
      }

      translate([0,0,-1]) hull() {
        translate([-d/2,0,0]) cylinder(r=r,h=caseH+2);
        translate([d/2,0,0]) cylinder(r=r,h=caseH+2);
      }
    }
  }
     translate([-36/2,0,r+0.5])  cube([36,40,2]);
    }
     translate([-34/2,-1,r-1])  cube([34,42,2]);
    
    hull() {
      translate([-12.5,pokerR,-0.1]) cylinder(r=pokerR+0.5,h=15+0.2);
      translate([12.5,pokerR,-0.1]) cylinder(r=pokerR+0.5,h=15+0.2);
      translate([-34/2,0,-0.1]) cube([34,pokerR,15+0.2]);
    }
  }

}

module presserShape() {
  o=1;
  translate([0,0,-0.1]) {
 translate([-12.5,0,0])  cylinder(r=pokerR+0.6,h=100);
 translate([12.5,0,0])  cylinder(r=pokerR+0.6,h=100);
 translate([-12.3,-1,0]) cube([25,2,100]);
 translate([-5,0.9,0]) cube([10,5.2,100]);
  translate([-6,3.9,0]) cube([15.4,5.2,100]);
    translate([-7+o,7.9,0]) cube([23,23,100]);
translate([-12+o,11.9,0]) cube([33,3,100]);
  }
color("red") translate([-20,24.6,3]) {
  cube([20,2,20]);
   cube([5,10,20]);
}

}

module torus(r1,r2) {
  rotate_extrude(convexity = 10, $fn=$fn)
    translate([r1, 0, 0])
    circle(r = r2, $fn=$fn);
}
