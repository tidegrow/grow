/*
 * Tidegrow Grow
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * required hardware:
 *  - 4 x M3 x 25mm countersunk bolts
 *  - 4 x M3 nuts
 *  - 4 x M3 countersunk magnets
 *
 */
 
$fn = 36;

tgg_pcb();
translate([0,50+2.5,0]) tgg_pcb();

//tgp_power();

translate([-100+12.5+2,0,1.6]) tgg_socket();
translate([100,0,(14/2)+1.9]) tgg_lens_left();

translate([100-(14/2)+2,0,(14/2)+1.6]) tgg_power();
translate([100,0,(14/2)+1.9]) tgg_lens_right();

//translate([100-5,-(25+1.25),-5]) tgg_coupler();
//translate([100-5,(25+1.25),-5]) tgg_coupler();

 
module tgg_pcb() {
     difference() {
        union() {
            color([0,1,0]) rcube(200,50,1.4,5);
            // zwölf
            color([0.3,0.3,0.3]) translate([-100+(7.5/2),0,1.4+3/2]) cube([7.5,16.5,3], center=true);
            // zwölf springs
            color([0.7,0.7,0.7]) translate([-100+(7.5/2),0,1.4+4]) cube([5,15,3], center=true);
            // power sockets
            color([0.2,0.2,0.2]) translate([100-(12/2),0,1.4+5.55]) cube([12,25,11.1], center=true);
        }
        translate([-100+5,25-5,-5]) cylinder(d=3, h=10);
        translate([100-5,25-5,-5]) cylinder(d=3, h=10);
        translate([-100+5,-25+5,-5]) cylinder(d=3, h=10);
        translate([100-5,-25+5,-5]) cylinder(d=3, h=10);
     }
}
 
module tgg_socket() {
  
     difference() {
        union() {
            translate([-2,0,2.5]) rcube(25,50,5,5);
            translate([-9.5,25-5,0]) cylinder(d=10, h=12.5);
            translate([-9.5,-25+5,0]) cylinder(d=10, h=12.5);
        }
        translate([-9.5,25-5,-5]) cylinder(d=3.75, h=20);
        translate([-9.5,-25+5,-5]) cylinder(d=3.75, h=20);
        translate([2,0,1.2]) rotate([0,0,0]) tgg_socket_cutout();
        
        translate([12.5,0,-5]) cylinder(d=30, h=20);
        
     }
        
}
 
module tgg_socket_cutout() {
                
    translate([-13,0,2.25]) cube([10,16.5,4.5], center=true);      
    translate([-18,0,2.25]) cube([10,16.5,20], center=true);                    
 
}
 
module tgg_power() {
    difference() {
        union() {
            translate([0,0,3.3]) rcube(10,50,4,5);
            translate([0,25-5,-7.6]) cylinder(d=10, h=12.5);
            translate([0,-25+5,-7.6]) cylinder(d=10, h=12.5);
        }
        
        translate([0,25-5,-10]) cylinder(d=3.75, h=25);
        translate([0,-25+5,-10]) cylinder(d=3.75, h=25); 
    }
}

module tgp_power() { // bloom version
    difference() {
        union() {
            translate([0,0,3.3]) rcube(10,100,4,5);
            translate([0,45,-7.6]) cylinder(d=10, h=12.5);
            translate([0,-45,-7.6]) cylinder(d=10, h=12.5);
        }
        
        translate([0,45,-10]) cylinder(d=3.75, h=25);
        translate([0,-45,-10]) cylinder(d=3.75, h=25); 
    }
}


 module tgg_coupler() {
     difference() {
        rcube(10,24.5,5,5);
        translate([0,(5+1.25),-5]) cylinder(d=3.75, h=20);
        translate([0,-(5+1.25),-5]) cylinder(d=3.75, h=20);
     }
}
 
module tgg_lens_left() {
    
    difference() {
        union() {
            color([1,0,0]) translate([-150,0,1]) rcube(100,50,6,5);
            translate([-187.5,0,0]) rcube(25,50,3.5,5);
        }
        
        translate([-100,0,-12-0.75]) rcube(176,45,20-1,5);
        
        translate([-5,25-5,-5]) cylinder(d=3.75, h=20);
        translate([-5,-25+5,-5]) cylinder(d=3.75, h=20);

        translate([-195,25-5,-10]) cylinder(d=3.75, h=25);
        translate([-195,-25+5,-10]) cylinder(d=3.75, h=25); 
        
    }
}

module tgg_lens_right() {
    
    difference() {
        union() {
            color([1,0,0]) translate([-50,0,1]) rcube(100,50,6,5);
        }
        
        translate([-10,0,-12-0.75]) rcube(176,40,20-1,5);
        
        translate([-100,0,-12-0.75]) rcube(176,45,20-1,5);
        
        translate([-5,25-5,-5]) cylinder(d=3.75, h=20);
        translate([-5,-25+5,-5]) cylinder(d=3.75, h=20);

        translate([-195,25-5,-10]) cylinder(d=3.75, h=25);
        translate([-195,-25+5,-10]) cylinder(d=3.75, h=25); 
        
    }
}


 module rcube(xx, yy, height, radius)
{
    translate([-xx/2,-yy/2,height/2])
    hull()
    {
        translate([radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,yy-radius,0])
        cylinder(height,radius,radius,true);

        translate([radius,yy-radius,0])
        cylinder(height,radius,radius,true);
    }
}