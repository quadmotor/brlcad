##
#                       G R I D . T C L
#
# Author -
#	Robert G. Parker
#
# Source -
#	The U. S. Army Research Laboratory
#	Aberdeen Proving Ground, Maryland  21005
#
# Distribution Notice -
#	Re-distribution of this software is restricted, as described in
#       your "Statement of Terms and Conditions for the Release of
#       The BRL-CAD Package" agreement.
#
# Description -
#	Control Panel for MGED's grid.
#

proc do_grid_spacing { id spacing_type } {
    global player_screen
    global grid_control_spacing
    global localunit

    set top .$id.grid_spacing

    if [winfo exists $top] {
	raise $top
	return
    }

    toplevel $top -screen $player_screen($id)

    frame $top.gridF1 -relief groove -bd 2
    frame $top.gridF2

    label $top.tickSpacingL -text "Tick Spacing\n($localunit/tick)"
    label $top.majorSpacingL -text "Major Spacing\n(ticks/major)"

    if {$spacing_type == "h"} {
	label $top.resL -text "Horiz." -anchor w
	entry $top.resE -relief sunken -width 12 -textvar grid_control_spacing($id,tick)
	entry $top.maj_resE -relief sunken -width 12 -textvar grid_control_spacing($id,ticksPerMajor)
    } elseif {$spacing_type == "v"} {
	label $top.resL -text "Vert." -anchor w
	entry $top.resE -relief sunken -width 12 -textvar grid_control_spacing($id,tick)
	entry $top.maj_resE -relief sunken -width 12 -textvar grid_control_spacing($id,ticksPerMajor)
    } elseif {$spacing_type == "b"} {
	label $top.resL -text "Horiz. & Vert." -anchor w
	entry $top.resE -relief sunken -width 12 -textvar grid_control_spacing($id,tick)
	entry $top.maj_resE -relief sunken -width 12 -textvar grid_control_spacing($id,ticksPerMajor)
    } else {
	catch {destroy $top}
	return
    }

    button $top.applyB -relief raised -text "Apply"\
	    -command "grid_spacing_apply $id $spacing_type"
    button $top.resetB -relief raised -text "Reset"\
	    -command "grid_spacing_reset $id $spacing_type"
    button $top.autosizeB -relief raised -text "Autosize"\
	    -command "grid_spacing_autosize $id"
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "catch { destroy $top }"

    grid x $top.tickSpacingL x $top.majorSpacingL -in $top.gridF1 -padx 8 -pady 8
    grid $top.resL $top.resE x $top.maj_resE -sticky "ew" -in $top.gridF1 -padx 8 -pady 8

    grid columnconfigure $top.gridF1 1 -weight 1
    grid columnconfigure $top.gridF1 3 -weight 1

    grid $top.applyB x $top.resetB $top.autosizeB x $top.dismissB -sticky "ew" -in $top.gridF2
    grid columnconfigure $top.gridF2 1 -weight 1
    grid columnconfigure $top.gridF2 1 -minsize 10
    grid columnconfigure $top.gridF2 4 -weight 1
    grid columnconfigure $top.gridF2 4 -minsize 10

    grid $top.gridF1 -sticky "ew" -padx 8 -pady 8
    grid $top.gridF2 -sticky "ew" -padx 8 -pady 8

    grid columnconfigure $top 0 -weight 1

    grid_spacing_reset $id $spacing_type

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]

    wm protocol $top WM_DELETE_WINDOW "catch { destroy $top }"
    wm geometry $top +$x+$y
    wm title $top "Grid Spacing ($id)"
}

proc do_grid_anchor { id } {
    global player_screen
    global mged_active_dm
    global grid_control_anchor

    set top .$id.grid_anchor

    if [winfo exists $top] {
	raise $top

	return
    }

    # Initialize variables
    winset $mged_active_dm($id)
    set grid_control_anchor($id) [_mged_rset grid anchor]

    toplevel $top -screen $player_screen($id)

    frame $top.gridF1 -relief groove -bd 2
    frame $top.gridF2

    frame $top.anchorF

    label $top.anchorL -text "Anchor Point" -anchor w
    entry $top.anchorE -relief sunken -bd 2 -width 12 -textvar grid_control_anchor($id)

    button $top.applyB -relief raised -text "Apply"\
	    -command "mged_apply $id \"rset grid anchor \\\$grid_control_anchor($id)\""
    button $top.resetB -relief raised -text "Reset"\
	    -command "winset \$mged_active_dm($id);\
	    set grid_control_anchor($id) \[rset grid anchor\]"
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "catch { destroy $top }"

    grid $top.anchorL -sticky "ew" -in $top.anchorF
    grid $top.anchorE -sticky "ew" -in $top.anchorF
    grid columnconfigure $top.anchorF 0 -weight 1

    grid $top.anchorF -sticky "ew" -in $top.gridF1 -padx 8 -pady 8
    grid columnconfigure $top.gridF1 0 -weight 1

    grid $top.applyB x $top.resetB x $top.dismissB -sticky "ew" -in $top.gridF2
    grid columnconfigure $top.gridF2 1 -weight 1
    grid columnconfigure $top.gridF2 1 -weight 3

    grid $top.gridF1 -sticky "ew" -padx 8 -pady 8
    grid $top.gridF2 -sticky "ew" -padx 8 -pady 8

    grid columnconfigure $top 0 -weight 1

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]

    wm protocol $top WM_DELETE_WINDOW "catch { destroy $top }"
    wm geometry $top +$x+$y
    wm title $top "Grid Anchor Point ($id)"
}

proc init_grid_control { id } {
    global player_screen
    global grid_control
    global mged_grid
    global localunit

    set top .$id.grid_control

    if [winfo exists $top] {
	raise $top

	return
    }

    toplevel $top -screen $player_screen($id)

    frame $top.gridF1
    frame $top.gridFF1 -relief groove -bd 2
    frame $top.gridF2
    frame $top.gridFF2 -relief groove -bd 2
    frame $top.gridF3
    frame $top.gridFF3 -relief groove -bd 2
    frame $top.gridF4

    frame $top.hF -relief sunken -bd 2
    frame $top.maj_hF -relief sunken -bd 2
    frame $top.vF -relief sunken -bd 2
    frame $top.maj_vF -relief sunken -bd 2

    frame $top.anchorF
    frame $top.anchorFF -relief sunken -bd 2

    label $top.tickSpacingL -text "Tick Spacing\n($localunit/tick)"
    label $top.majorSpacingL -text "Major Spacing\n(ticks/major)"

    label $top.hL -text "Horiz." -anchor w
    entry $top.hE -relief flat -width 12 -textvar grid_control($id,rh)
    menubutton $top.hMB -relief raised -bd 2\
	    -menu $top.hMB.spacing -indicatoron 1
    menu $top.hMB.spacing -tearoff 0
    $top.hMB.spacing add command -label "micrometer" -underline 4\
	    -command "set_grid_spacing $id micrometer 0"
    $top.hMB.spacing add command -label "millimeter" -underline 2\
	    -command "set_grid_spacing $id millimeter 0"
    $top.hMB.spacing add command -label "centimeter" -underline 0\
	    -command "set_grid_spacing $id centimeter 0"
    $top.hMB.spacing add command -label "decimeter" -underline 0\
	    -command "set_grid_spacing $id decimeter 0"
    $top.hMB.spacing add command -label "meter" -underline 0\
	    -command "set_grid_spacing $id meter 0"
    $top.hMB.spacing add command -label "kilometer" -underline 0\
	    -command "set_grid_spacing $id kilometer 0"
    $top.hMB.spacing add separator
    $top.hMB.spacing add command -label "1/10 inch" -underline 0\
	    -command "set_grid_spacing $id \"1/10 inch\" 0"
    $top.hMB.spacing add command -label "1/4 inch" -underline 2\
	    -command "set_grid_spacing $id \"1/4 inch\" 0"
    $top.hMB.spacing add command -label "1/2 inch" -underline 2\
	    -command "set_grid_spacing $id \"1/2 inch\" 0"
    $top.hMB.spacing add command -label "inch" -underline 0\
	    -command "set_grid_spacing $id inch 0"
    $top.hMB.spacing add command -label "foot" -underline 0\
	    -command "set_grid_spacing $id foot 0"
    $top.hMB.spacing add command -label "yard" -underline 0\
	    -command "set_grid_spacing $id yard 0"
    $top.hMB.spacing add command -label "mile" -underline 0\
	    -command "set_grid_spacing $id mile 0"
    entry $top.maj_hE -relief flat -width 12 -textvar grid_control($id,mrh)

    label $top.vL -text "Vert." -anchor w
    entry $top.vE -relief flat -width 12 -textvar grid_control($id,rv)
    menubutton $top.vMB -relief raised -bd 2\
	    -menu $top.vMB.spacing -indicatoron 1
    menu $top.vMB.spacing -tearoff 0
    $top.vMB.spacing add command -label "micrometer" -underline 4\
	    -command "set_grid_spacing $id micrometer 0"
    $top.vMB.spacing add command -label "millimeter" -underline 2\
	    -command "set_grid_spacing $id millimeter 0"
    $top.vMB.spacing add command -label "centimeter" -underline 0\
	    -command "set_grid_spacing $id centimeter 0"
    $top.vMB.spacing add command -label "decimeter" -underline 0\
	    -command "set_grid_spacing $id decimeter 0"
    $top.vMB.spacing add command -label "meter" -underline 0\
	    -command "set_grid_spacing $id meter 0"
    $top.vMB.spacing add command -label "kilometer" -underline 0\
	    -command "set_grid_spacing $id kilometer 0"
    $top.vMB.spacing add separator
    $top.vMB.spacing add command -label "1/10 inch" -underline 0\
	    -command "set_grid_spacing $id \"1/10 inch\" 0"
    $top.vMB.spacing add command -label "1/4 inch" -underline 2\
	    -command "set_grid_spacing $id \"1/4 inch\" 0"
    $top.vMB.spacing add command -label "1/2 inch" -underline 2\
	    -command "set_grid_spacing $id \"1/2 inch\" 0"
    $top.vMB.spacing add command -label "inch" -underline 0\
	    -command "set_grid_spacing $id inch 0"
    $top.vMB.spacing add command -label "foot" -underline 0\
	    -command "set_grid_spacing $id foot 0"
    $top.vMB.spacing add command -label "yard" -underline 0\
	    -command "set_grid_spacing $id yard 0"
    $top.vMB.spacing add command -label "mile" -underline 0\
	    -command "set_grid_spacing $id mile 0"
    entry $top.maj_vE -relief flat -width 12 -textvar grid_control($id,mrv)

    checkbutton $top.squareGridCB -relief flat -text "Square Grid"\
	    -offvalue 0 -onvalue 1 -variable grid_control($id,square)\
	    -command "set_grid_square $id"

    label $top.anchorL -text "Anchor Point" -anchor w
    entry $top.anchorE -relief flat -width 12 -textvar grid_control($id,anchor)

    label $top.gridEffectsL -text "Grid Effects" -anchor w

    checkbutton $top.drawCB -relief flat -text "Draw"\
	    -offvalue 0 -onvalue 1 -variable grid_control($id,draw)

    checkbutton $top.snapCB -relief flat -text "Snap"\
	    -offvalue 0 -onvalue 1 -variable grid_control($id,snap)

    button $top.applyB -relief raised -text "Apply"\
	    -command "grid_control_apply $id"
    button $top.resetB -relief raised -text "Reset"\
	    -command "grid_control_reset $id $top"
    button $top.autosizeB -relief raised -text "Autosize"\
	    -command "grid_control_autosize $id"
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "catch { destroy $top }"

    grid x $top.tickSpacingL x $top.majorSpacingL -in $top.gridFF1 -padx 8 -pady 8
    grid $top.hE $top.hMB -sticky ew -in $top.hF
    grid columnconfigure $top.hF 0 -weight 1
    grid $top.maj_hE -sticky ew -in $top.maj_hF
    grid columnconfigure $top.maj_hF 0 -weight 1
    grid $top.hL $top.hF x $top.maj_hF -sticky "ew" -in $top.gridFF1 -padx 8 -pady 8
    grid $top.vE $top.vMB -sticky ew -in $top.vF
    grid columnconfigure $top.vF 0 -weight 1
    grid $top.maj_vE -sticky ew -in $top.maj_vF
    grid columnconfigure $top.maj_vF 0 -weight 1
    grid $top.vL $top.vF x $top.maj_vF -sticky "ew" -in $top.gridFF1 -padx 8 -pady 8
    grid $top.squareGridCB - - - -in $top.gridFF1 -padx 8 -pady 8

    grid $top.anchorL -sticky "ew" -in $top.anchorF
    grid $top.anchorE -sticky "ew" -in $top.anchorFF
    grid $top.anchorFF -sticky "ew" -in $top.anchorF
    grid $top.anchorF x x -sticky "ew" -in $top.gridFF2 -padx 8 -pady 8
    grid columnconfigure $top.anchorF 0 -weight 1
    grid columnconfigure $top.anchorFF 0 -weight 1

    grid $top.gridEffectsL x $top.drawCB x $top.snapCB x -sticky "ew" -in $top.gridFF3\
	    -padx 8 -pady 8

    grid $top.applyB x $top.resetB $top.autosizeB x $top.dismissB -sticky "ew" -in $top.gridF4

    grid columnconfigure $top.gridFF1 1 -weight 1
    grid columnconfigure $top.gridFF1 3 -weight 1
    grid columnconfigure $top.gridF1 0 -weight 1
    grid $top.gridFF1 -sticky "ew" -in $top.gridF1 -padx 8 -pady 8
    grid $top.gridF1 -sticky "ew" -padx 8 -pady 8
    

    grid columnconfigure $top.gridFF2 0 -weight 1
    grid columnconfigure $top.gridFF2 1 -minsize 20
    grid columnconfigure $top.gridFF2 2 -weight 1
    grid columnconfigure $top.gridF2 0 -weight 1
    grid $top.gridFF2 -sticky "ew" -in $top.gridF2 -padx 8
    grid $top.gridF2 -sticky "ew" -padx 8 -pady 8

    grid columnconfigure $top.gridFF3 0 -weight 0
    grid columnconfigure $top.gridFF3 1 -weight 1
    grid columnconfigure $top.gridFF3 3 -minsize 20
    grid columnconfigure $top.gridFF3 5 -weight 1
    grid columnconfigure $top.gridF3 0 -weight 1
    grid $top.gridFF3 -sticky "ew" -in $top.gridF3 -padx 8 -pady 8
    grid $top.gridF3 -sticky "ew" -padx 8 -pady 8

    grid columnconfigure $top.gridF4 1 -weight 1
    grid columnconfigure $top.gridF4 4 -weight 1
    grid $top.gridF4 -sticky "ew" -padx 8 -pady 8

    grid columnconfigure $top 0 -weight 1
    grid columnconfigure $top 0 -minsize 400

    grid_control_reset $id $top
    set grid_control($id,square) 1
    set_grid_square $id

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]

    wm protocol $top WM_DELETE_WINDOW "catch { destroy $top }"
    wm geometry $top +$x+$y
    wm title $top "Grid Control Panel ($id)"
}

proc grid_control_apply { id } {
    global grid_control
    global mged_grid

    mged_apply $id "rset grid anchor \$grid_control($id,anchor)"
    mged_apply $id "rset grid rh \$grid_control($id,rh)"
    mged_apply $id "rset grid mrh \$grid_control($id,mrh)"

    if {$grid_control($id,square)} {
	mged_apply $id "rset grid rv \$grid_control($id,rh)"
	mged_apply $id "rset grid mrv \$grid_control($id,mrh)"
	set grid_control($id,rv) $grid_control($id,rh)
	set grid_control($id,mrv) $grid_control($id,mrh)
    } else {
	mged_apply $id "rset grid rv \$grid_control($id,rv)"
	mged_apply $id "rset grid mrv \$grid_control($id,mrv)"
    }

    mged_apply $id "rset grid snap \$grid_control($id,snap)"
    mged_apply $id "rset grid draw \$grid_control($id,draw)"

    # update the main GUI
    set mged_grid($id,draw) $grid_control($id,draw)
    set mged_grid($id,snap) $grid_control($id,snap)
}

proc grid_control_reset { id top } {
    global mged_active_dm
    global mged_grid
    global grid_control

    winset $mged_active_dm($id)

    set grid_control($id,draw) [rset grid draw]
    set grid_control($id,snap) [rset grid snap]
    set grid_control($id,anchor) [rset grid anchor]
    set grid_control($id,rh) [rset grid rh]
    set grid_control($id,mrh) [rset grid mrh]
    set grid_control($id,rv) [rset grid rv]
    set grid_control($id,mrv) [rset grid mrv]

    if {$grid_control($id,rh) != $grid_control($id,rv) ||\
	$grid_control($id,mrh) != $grid_control($id,mrv)} {
	set grid_control($id,square) 0
	set_grid_square $id
    }

    set mged_grid($id,draw) $grid_control($id,draw)
    set mged_grid($id,snap) $grid_control($id,snap)
}

proc set_grid_square { id } {
    global grid_control

    set top .$id.grid_control
    if [winfo exists $top] {
	if {$grid_control($id,square)} {
	    $top.vE configure -textvar grid_control($id,rh)
	    $top.maj_vE configure -textvar grid_control($id,mrh)
	} else {
	    $top.vE configure -textvar grid_control($id,rv)
	    $top.maj_vE configure -textvar grid_control($id,mrv)
	}
    }
}

proc grid_control_update { sf } {
    global mged_players
    global grid_control
    global localunit

    foreach id $mged_players {
	if {[info exists grid_control($id,anchor)] &&\
		[llength $grid_control($id,anchor)] == 3} {
	    set x [lindex $grid_control($id,anchor) 0]
	    set y [lindex $grid_control($id,anchor) 1]
	    set z [lindex $grid_control($id,anchor) 2]

	    set x [expr $sf * $x]
	    set y [expr $sf * $y]
	    set z [expr $sf * $z]

	    set grid_control($id,anchor) "$x $y $z"
	}

	if [info exists grid_control($id,rh)] {
	    set grid_control($id,rh) [expr $sf * $grid_control($id,rh)]
	    set grid_control($id,rv) [expr $sf * $grid_control($id,rv)]
	}

	set top .$id.grid_control
	if [winfo exists $top] {
	    $top.tickSpacingL configure -text "Tick Spacing\n($localunit/tick)"
	}

	set top .$id.grid_spacing
	if [winfo exists $top] {
	    $top.tickSpacingL configure -text "Tick Spacing\n($localunit/tick)"
	}
    }
}

proc grid_autosize {} {
# Gives between 20 and 200 ticks in user units
    set lower [expr log10(20)]
    set upper [expr $lower+1]
    set s [expr log10([_mged_view size])]

    if {$s < $lower} {
	set val [expr pow(10, floor($s - $lower))]
    } elseif {$upper < $s} {
	set val [expr pow(10, ceil($s - $upper))]
    } else {
	set val 1.0
    }

    return $val
}

proc grid_spacing_autosize { id } {
    global grid_control_spacing

    set val [grid_autosize]

    set grid_control_spacing($id,tick) $val
    set grid_control_spacing($id,ticksPerMajor) 10
}

proc grid_control_autosize { id } {
    global grid_control

    set val [grid_autosize]

    set grid_control($id,rh) $val
    set grid_control($id,rv) $val
    set grid_control($id,mrh) 10
    set grid_control($id,mrv) 10
}

proc grid_spacing_apply { id spacing_type } {
    global mged_active_dm
    global grid_control_spacing

    winset $mged_active_dm($id)

    if {$spacing_type == "h"} {
	rset grid rh $grid_control_spacing($id,tick)
	rset grid mrh $grid_control_spacing($id,ticksPerMajor)
    } elseif {$spacing_type == "v"} {
	rset grid rv $grid_control_spacing($id,tick)
	rset grid mrv $grid_control_spacing($id,ticksPerMajor)
    } else {
	rset grid rh $grid_control_spacing($id,tick)
	rset grid mrh $grid_control_spacing($id,ticksPerMajor)
	rset grid rv $grid_control_spacing($id,tick)
	rset grid mrv $grid_control_spacing($id,ticksPerMajor)
    }

    catch { destroy .$id.grid_spacing }
}

proc grid_spacing_reset { id spacing_type } {
    global mged_active_dm
    global grid_control_spacing

    winset $mged_active_dm($id)

    if {$spacing_type == "v"} {
	set grid_control_spacing($id,tick) [rset grid rv]
	set grid_control_spacing($id,ticksPerMajor) [rset grid mrv]
    } else {
	set grid_control_spacing($id,tick) [rset grid rh]
	set grid_control_spacing($id,ticksPerMajor) [rset grid mrh]
    }
}

proc set_grid_spacing { id grid_unit apply } {
    global base2local
    global grid_control

    switch $grid_unit {
	micrometer {
	    set res [expr 0.001 * $base2local]
	    set res_major 10
	}
	millimeter {
	    set res $base2local
	    set res_major 10
	}
	centimeter {
	    set res [expr 10 * $base2local]
	    set res_major 10
	}
	decimeter {
	    set res [expr 100 * $base2local]
	    set res_major 10
	}
	meter {
	    set res [expr 1000 * $base2local]
	    set res_major 10
	}
	kilometer {
	    set res [expr 1000000 * $base2local]
	    set res_major 10
	}
	"1/10 inch" {
	    set res [expr 2.54 * $base2local]
	    set res_major 10
	}
	"1/4 inch" {
	    set res [expr 6.35 * $base2local]
	    set res_major 4
	}
	"1/2 inch" {
	    set res [expr 12.7 * $base2local]
	    set res_major 2
	}
	inch {
	    set res [expr 25.4 * $base2local]
	    set res_major 12
	}
	foot {
	    set res [expr 304.8 * $base2local]
	    set res_major 10
	}
	yard {
	    set res [expr 914.4 * $base2local]
	    set res_major 10
	}
	mile {
	    set res [expr 1609344 * $base2local]
	    set res_major 10
	}
    }

    if {$apply} {
	mged_apply $id "rset grid rh $res"
	mged_apply $id "rset grid rv $res"
	mged_apply $id "rset grid mrh $res_major"
	mged_apply $id "rset grid mrv $res_major"
    } else {
	set grid_control($id,rh) $res
	set grid_control($id,rv) $res
	set grid_control($id,mrh) $res_major
	set grid_control($id,mrv) $res_major
    }
}
