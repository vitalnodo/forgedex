; ══════════════════════════════════════════════════════════
;  Raycaster — a grid, and one ray per column of the screen
;
;  class Board extends View {
;      int[] px;  Bitmap bmp;  int bw, bh;  String map;
;      double posX, posY, dirX, dirY, planeX, planeY;
;
;      void cast() {                       // one frame
;          for (int x = 0; x < bw; x++) {
;              ...a ray through column x, stepped square by square
;                 until it meets a wall, then a vertical strip whose
;                 height is the reciprocal of the distance...
;          }
;          bmp.setPixels(px, ...);
;      }
;      void move(double d), turn(double a);
;      boolean solid(int x, int y);
;  }
;
;  There is no perspective transform and no polygon anywhere: the
;  whole illusion is that a wall the ray met at distance d is drawn
;  h/d pixels tall.  Everything the frame costs is one ray per
;  column, so the work grows with the width of the screen and not
;  with the size of the world.
;
;  The map is the picture below, read as one string of 256
;  characters — sixteen rows of sixteen.  solid() indexes it
;  directly, which is why the world costs no memory to speak of.
;
;      ################
;      #..............#
;      #..###...####..#
;      #..#.......#...#
;      #..#..###..#.###
;      #.....#.#......#
;      #..####.####...#
;      #..............#
;      #..##..####..###
;      #..#......#....#
;      #..#..##..#..#.#
;      #.....##.....#.#
;      #..######....#.#
;      #............#.#
;      #..............#
;      ################
;
;  On the registers: the nibble formats (12x, 22c, 35c) reach only
;  v0..v15, and this frame holds sixteen live doubles.  So the low
;  registers are kept as a landing pad — v0:v1 for a double, v2 for
;  an int — and the three macros below spell out the shuttle
;  through it once instead of at each of its thirty-odd uses.
;  Everything else is 23x and takes a whole byte per register, so
;  the arithmetic itself is unconstrained.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

SHRINK  = $04           ; render at a quarter of the view, scale up
SIDE    = $10           ; the map is SIDE by SIDE

; ── The shuttle through the low registers ─────────────────
;
;  All three clobber v0:v1 and v2, and all three are only usable
;  where v3 holds `this` — which is to say, inside cast.

; a double out of a field, into a register too high to iget into
macro load_wide? dst?*, obj?*, ref?*&
    iget_wide        v0, obj, ref
    move_wide_from16 dst, v0
end macro

; an int widened into a double, either of which may be out of reach
macro from_int? dst?*, src?*
    move             v2, src
    int_to_double    v0, v2
    move_wide_from16 dst, v0
end macro

; and back the other way
macro to_int? dst?*, src?*
    move_wide_from16 v0, src
    double_to_int    dst, v0
end macro

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _fields
    ; field references are not auto-registered from the code, so
    ; this enum constant is spelled out
    deffield "Landroid/graphics/Bitmap$Config;->ARGB_8888:Landroid/graphics/Bitmap$Config;"
end macro

macro _board
    defclass "Lapp/hello/ray/Board;", ACC_PUBLIC, "Landroid/view/View;"

    deffield "->px:[I", ACC_PUBLIC
    deffield "->bmp:Landroid/graphics/Bitmap;", ACC_PUBLIC
    deffield "->blit:Landroid/graphics/Paint;", ACC_PUBLIC
    deffield "->map:Ljava/lang/String;", ACC_PUBLIC
    deffield "->bw:I", ACC_PUBLIC
    deffield "->bh:I", ACC_PUBLIC
    deffield "->posX:D", ACC_PUBLIC
    deffield "->posY:D", ACC_PUBLIC
    deffield "->dirX:D", ACC_PUBLIC
    deffield "->dirY:D", ACC_PUBLIC
    deffield "->planeX:D", ACC_PUBLIC
    deffield "->planeY:D", ACC_PUBLIC

    ; v0=paint  v1=scratch  v2:v3=a coordinate
    method "-><init>(Landroid/content/Context;)V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 4
        invoke_direct  "Landroid/view/View;-><init>(Landroid/content/Context;)V", p0, p1

        new_instance   v0, "Landroid/graphics/Paint;"
        invoke_direct  "Landroid/graphics/Paint;-><init>()V", v0
        const_4        v1, 1
        invoke_virtual "Landroid/graphics/Paint;->setFilterBitmap(Z)V", v0, v1
        iput_object    v0, p0, "->blit:Landroid/graphics/Paint;"

        const_string   v0, "#################..............##..###...####..##..#.......#...##..#..###..#.####.....#.#......##..####.####...##..............##..##..####..####..#......#....##..#..##..#..#.##.....##.....#.##..######....#.##............#.##..............#################"
        iput_object    v0, p0, "->map:Ljava/lang/String;"

        ; standing in the open corner, looking along the row
        const_wide     v2, 1.5
        iput_wide      v2, p0, "->posX:D"
        const_wide     v2, 1.5
        iput_wide      v2, p0, "->posY:D"
        const_wide     v2, 1.0
        iput_wide      v2, p0, "->dirX:D"
        const_wide     v2, 0.0
        iput_wide      v2, p0, "->dirY:D"
        const_wide     v2, 0.0
        iput_wide      v2, p0, "->planeX:D"
        ; two thirds of the direction's length, which is a field of
        ; view of about sixty-six degrees
        const_wide     v2, 0.66
        iput_wide      v2, p0, "->planeY:D"
        return_void
    end method

    ; Anything off the edge counts as a wall, so the ray always
    ; meets something and the stepping loop always ends.
    ;
    ; v0=map  v1=index then character  v2=scratch  v3=answer
    method "->solid(II)Z", ACC_PUBLIC, locals 4
        const_4        v3, 1

        if_ltz         p1, .wall
        if_ltz         p2, .wall
        const          v2, SIDE
        sub_int        v1, p1, v2
        if_gez         v1, .wall
        sub_int        v1, p2, v2
        if_gez         v1, .wall

        iget_object    v0, p0, "->map:Ljava/lang/String;"
        mul_int        v1, p2, v2
        add_int        v1, v1, p1
        invoke_virtual "Ljava/lang/String;->charAt(I)C", v0, v1
        move_result    v1
        const          v2, $23                          ; '#'
        if_eq          v1, v2, .wall
        const_4        v3, 0

    .wall:
        return         v3
    end method

    ; ── One frame ────────────────────────────────────────
    ;
    ;  v0:v1  the double landing pad     v2  the int landing pad
    ;  v3=this  v4=px  v5=width  v6=height
    ;  v7=column  v8=row
    ;  v9=mapX  v10=mapY  v11=stepX  v12=stepY  v13=side
    ;  v14,v15 scratch ints
    ;  v16:v17 posX     v18:v19 posY
    ;  v20:v21 dirX     v22:v23 dirY
    ;  v24:v25 planeX   v26:v27 planeY
    ;  v28:v29 cameraX
    ;  v30:v31 rayDirX  v32:v33 rayDirY
    ;  v34:v35 deltaX   v36:v37 deltaY
    ;  v38:v39 sideDistX  v40:v41 sideDistY
    ;  v42:v43 the perpendicular distance to the wall
    ;  v44:v45 scratch
    ;  v48:v49 1.0  v50:v51 2.0  v52:v53 0.0
    ;  v54:v55 the width as a double  v56:v57 the height
    ;  v58=top of the strip  v59=bottom  v60=its colour
    ;  v61=scratch int
    method "->cast()V", ACC_PUBLIC, locals 64
        move_object_from16 v3, p0

        iget           v5, v3, "->bw:I"
        iget           v6, v3, "->bh:I"
        iget_object    v4, v3, "->px:[I"

        load_wide      v16, v3, "->posX:D"
        load_wide      v18, v3, "->posY:D"
        load_wide      v20, v3, "->dirX:D"
        load_wide      v22, v3, "->dirY:D"
        load_wide      v24, v3, "->planeX:D"
        load_wide      v26, v3, "->planeY:D"

        const_wide     v48, 1.0
        const_wide     v50, 2.0
        const_wide     v52, 0.0
        from_int       v54, v5
        from_int       v56, v6

        const_4        v7, 0
    .column:
        sub_int        v14, v7, v5
        if_gez         v14, .done

        ; cameraX runs from -1 at the left edge to +1 at the right
        from_int       v28, v7
        mul_double     v28, v28, v50
        div_double     v28, v28, v54
        sub_double     v28, v28, v48

        mul_double     v44, v24, v28
        add_double     v30, v20, v44                    ; rayDirX
        mul_double     v44, v26, v28
        add_double     v32, v22, v44                    ; rayDirY

        to_int         v9, v16
        to_int         v10, v18

        ; How far the ray travels for one whole square in each
        ; axis.  A ray exactly along an axis divides by zero here
        ; and gets an infinity, which is the right answer: that
        ; side is never the nearer one and never gets stepped.
        div_double     v34, v48, v30
        invoke_static_range "Ljava/lang/Math;->abs(D)D", v34, v35
        move_result_wide v34
        div_double     v36, v48, v32
        invoke_static_range "Ljava/lang/Math;->abs(D)D", v36, v37
        move_result_wide v36

        ; which way each axis steps, and how far to its first edge
        cmpl_double    v14, v30, v52
        if_gez         v14, .east
        const_4        v11, -1
        from_int       v44, v9
        sub_double     v44, v16, v44
        mul_double     v38, v44, v34
        goto           .have_x
    .east:
        const_4        v11, 1
        from_int       v44, v9
        add_double     v44, v44, v48
        sub_double     v44, v44, v16
        mul_double     v38, v44, v34
    .have_x:

        cmpl_double    v14, v32, v52
        if_gez         v14, .south
        const_4        v12, -1
        from_int       v44, v10
        sub_double     v44, v18, v44
        mul_double     v40, v44, v36
        goto           .have_y
    .south:
        const_4        v12, 1
        from_int       v44, v10
        add_double     v44, v44, v48
        sub_double     v44, v44, v18
        mul_double     v40, v44, v36
    .have_y:

        ; step to whichever edge is nearer, over and over
    .step:
        cmpg_double    v14, v38, v40
        if_gez         v14, .step_y
        add_double     v38, v38, v34
        add_int        v9, v9, v11
        const_4        v13, 0
        goto           .probe
    .step_y:
        add_double     v40, v40, v36
        add_int        v10, v10, v12
        const_4        v13, 1
    .probe:
        invoke_virtual "Lapp/hello/ray/Board;->solid(II)Z", v3, v9, v10
        move_result    v14
        if_eqz         v14, .step

        ; Measure to the wall along the view direction, not along
        ; the ray: taking the ray's own length would bow the walls
        ; outward at the edges of the screen.
        if_nez         v13, .by_y
        sub_double     v42, v38, v34
        goto           .have_dist
    .by_y:
        sub_double     v42, v40, v36
    .have_dist:

        div_double     v44, v56, v42
        to_int         v14, v44                         ; strip height
        div_int_lit8   v14, v14, 2
        div_int_lit8   v15, v6, 2                       ; the horizon
        sub_int        v58, v15, v14
        add_int        v59, v15, v14

        if_gez         v58, .top_ok
        const          v58, $00
    .top_ok:
        sub_int        v61, v59, v6
        if_ltz         v61, .bottom_ok
        add_int_lit8   v59, v6, -1
    .bottom_ok:

        ; the two faces of a wall get different shades, which is
        ; the only thing that makes a corner visible
        if_nez         v13, .darker
        const          v60, $FFCC5533
        goto           .paint
    .darker:
        const          v60, $FF8A3B24
    .paint:

        const          v8, $00
    .pixel:
        sub_int        v61, v8, v6
        if_gez         v61, .next_column

        sub_int        v61, v8, v58
        if_ltz         v61, .sky
        sub_int        v61, v8, v59
        if_gtz         v61, .ground
        move_from16    v61, v60
        goto           .put
    .sky:
        const          v61, $FF20303C
        goto           .put
    .ground:
        const          v61, $FF3A3128
    .put:
        mul_int        v62, v8, v5
        add_int        v62, v62, v7
        aput           v61, v4, v62
        add_int_lit8   v8, v8, 1
        goto           .pixel

    .next_column:
        add_int_lit8   v7, v7, 1
        ; a column's worth of body is well past what the byte in a
        ; plain goto reaches
        goto_16        .column

    .done:
        invoke_virtual "Lapp/hello/ray/Board;->push()V", v3
        return_void
    end method

    ; The pixels into the bitmap — seven arguments and a receiver,
    ; so the call is a range one.
    method "->push()V", ACC_PUBLIC, locals 8
        iget_object    v0, p0, "->bmp:Landroid/graphics/Bitmap;"
        iget_object    v1, p0, "->px:[I"
        const_4        v2, 0
        iget           v3, p0, "->bw:I"
        const_4        v4, 0
        const_4        v5, 0
        iget           v6, p0, "->bw:I"
        iget           v7, p0, "->bh:I"
        invoke_virtual_range "Landroid/graphics/Bitmap;->setPixels([IIIIIII)V", v0, v7
        return_void
    end method

    ; Forward or back along the direction, unless a wall is there.
    ;
    ; v0:v1=posX  v2:v3=posY  v4:v5=dirX  v6:v7=dirY
    ; v8:v9=where the step would land, x  v10:v11=y
    method "->move(D)V", ACC_PUBLIC, locals 12
        iget_wide      v0, p0, "->posX:D"
        iget_wide      v2, p0, "->posY:D"
        iget_wide      v4, p0, "->dirX:D"
        iget_wide      v6, p0, "->dirY:D"

        mul_double     v8, v4, p1
        add_double     v8, v0, v8
        mul_double     v10, v6, p1
        add_double     v10, v2, v10

        double_to_int  v4, v8
        double_to_int  v5, v10
        invoke_virtual "Lapp/hello/ray/Board;->solid(II)Z", p0, v4, v5
        move_result    v6
        if_nez         v6, .blocked

        iput_wide      v8, p0, "->posX:D"
        iput_wide      v10, p0, "->posY:D"
    .blocked:
        return_void
    end method

    ; Rotate the direction and the camera plane together — turning
    ; one without the other would shear the view.
    ;
    ; v0:v1=cos  v2:v3=sin  v4:v5,v6:v7=the pair being turned
    ; v8:v9,v10:v11=scratch  v12:v13=the turned x  v15=this
    method "->turn(D)V", ACC_PUBLIC, locals 16
        move_object_from16 v15, p0
        invoke_static_range "Ljava/lang/Math;->cos(D)D", p1, p2
        move_result_wide v0
        invoke_static_range "Ljava/lang/Math;->sin(D)D", p1, p2
        move_result_wide v2

        iget_wide      v4, v15, "->dirX:D"
        iget_wide      v6, v15, "->dirY:D"
        mul_double     v12, v4, v0
        mul_double     v8, v6, v2
        sub_double     v12, v12, v8
        mul_double     v10, v4, v2
        mul_double     v8, v6, v0
        add_double     v10, v10, v8
        iput_wide      v12, v15, "->dirX:D"
        iput_wide      v10, v15, "->dirY:D"

        iget_wide      v4, v15, "->planeX:D"
        iget_wide      v6, v15, "->planeY:D"
        mul_double     v12, v4, v0
        mul_double     v8, v6, v2
        sub_double     v12, v12, v8
        mul_double     v10, v4, v2
        mul_double     v8, v6, v0
        add_double     v10, v10, v8
        iput_wide      v12, v15, "->planeX:D"
        iput_wide      v10, v15, "->planeY:D"
        return_void
    end method

    ; v0=width  v1=height  v2=array/length  v3=config then bitmap
    method "->setup()V", ACC_PUBLIC, locals 4
        invoke_virtual "Landroid/view/View;->getWidth()I", p0
        move_result    v0
        div_int_lit8   v0, v0, SHRINK
        invoke_virtual "Landroid/view/View;->getHeight()I", p0
        move_result    v1
        div_int_lit8   v1, v1, SHRINK
        iput           v0, p0, "->bw:I"
        iput           v1, p0, "->bh:I"

        mul_int        v2, v0, v1
        new_array      v2, v2, "[I"
        iput_object    v2, p0, "->px:[I"

        sget_object    v3, "Landroid/graphics/Bitmap$Config;->ARGB_8888:Landroid/graphics/Bitmap$Config;"
        invoke_static  "Landroid/graphics/Bitmap;->createBitmap(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;", v0, v1, v3
        move_result_object v3
        iput_object    v3, p0, "->bmp:Landroid/graphics/Bitmap;"

        invoke_virtual "Lapp/hello/ray/Board;->cast()V", p0
        return_void
    end method

    ; v0=bitmap  v1=paint  v2,v3=scale and origin
    method "->onDraw(Landroid/graphics/Canvas;)V", ACC_PUBLIC, locals 4
        iget_object    v0, p0, "->bmp:Landroid/graphics/Bitmap;"
        if_nez         v0, .ready
        invoke_virtual "Lapp/hello/ray/Board;->setup()V", p0
    .ready:
        invoke_virtual "Landroid/graphics/Canvas;->save()I", p1
        invoke_virtual "Landroid/view/View;->getWidth()I", p0
        move_result    v2
        int_to_float   v2, v2
        iget           v3, p0, "->bw:I"
        int_to_float   v3, v3
        div_float      v2, v2, v3
        invoke_virtual "Landroid/graphics/Canvas;->scale(FF)V", p1, v2, v2

        iget_object    v0, p0, "->bmp:Landroid/graphics/Bitmap;"
        iget_object    v1, p0, "->blit:Landroid/graphics/Paint;"
        const_4        v2, 0
        invoke_virtual "Landroid/graphics/Canvas;->drawBitmap(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V", p1, v0, v2, v2, v1
        invoke_virtual "Landroid/graphics/Canvas;->restore()V", p1
        return_void
    end method
end macro

macro _main
    defclass "Lapp/hello/ray/Main;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _main_ifaces

    deffield "->board:Lapp/hello/ray/Board;", ACC_PUBLIC
    deffield "->left:Landroid/widget/Button;", ACC_PUBLIC
    deffield "->right:Landroid/widget/Button;", ACC_PUBLIC
    deffield "->ahead:Landroid/widget/Button;", ACC_PUBLIC

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=column  v1=board  v2=button row  v3=a button  v4=scratch
    ; v5=layout parameters
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 6
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        new_instance   v0, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v0, p0
        const_4        v4, 1                                   ; VERTICAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v0, v4

        new_instance   v1, "Lapp/hello/ray/Board;"
        invoke_direct  "Lapp/hello/ray/Board;-><init>(Landroid/content/Context;)V", v1, p0
        iput_object    v1, p0, "->board:Lapp/hello/ray/Board;"

        new_instance   v5, "Landroid/widget/LinearLayout$LayoutParams;"
        const_4        v4, -1                                  ; MATCH_PARENT
        const_4        v3, 0
        const_float    v2, 1.0
        invoke_direct  "Landroid/widget/LinearLayout$LayoutParams;-><init>(IIF)V", v5, v4, v3, v2
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V", v0, v1, v5

        new_instance   v2, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v2, p0
        const_4        v4, 0                                   ; HORIZONTAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v2, v4

        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0
        const_string   v4, "  <  "
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0
        iput_object    v3, p0, "->left:Landroid/widget/Button;"
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v2, v3

        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0
        const_string   v4, "  ^  "
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0
        iput_object    v3, p0, "->ahead:Landroid/widget/Button;"
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v2, v3

        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0
        const_string   v4, "  v  "
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v2, v3

        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0
        const_string   v4, "  >  "
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0
        iput_object    v3, p0, "->right:Landroid/widget/Button;"
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v2, v3

        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v2
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method

    ; Three of the four buttons are kept in fields; whatever is not
    ; one of those is the fourth.
    ;
    ; v0=board  v1=the button being compared  v2:v3=how far
    method "->onClick(Landroid/view/View;)V", ACC_PUBLIC, locals 4
        iget_object    v0, p0, "->board:Lapp/hello/ray/Board;"

        iget_object    v1, p0, "->left:Landroid/widget/Button;"
        if_eq          v1, p1, .left
        iget_object    v1, p0, "->right:Landroid/widget/Button;"
        if_eq          v1, p1, .right
        iget_object    v1, p0, "->ahead:Landroid/widget/Button;"
        if_eq          v1, p1, .ahead

        const_wide     v2, -0.3
        invoke_virtual "Lapp/hello/ray/Board;->move(D)V", v0, v2, v3
        goto           .again
    .ahead:
        const_wide     v2, 0.3
        invoke_virtual "Lapp/hello/ray/Board;->move(D)V", v0, v2, v3
        goto           .again
    .left:
        const_wide     v2, -0.25
        invoke_virtual "Lapp/hello/ray/Board;->turn(D)V", v0, v2, v3
        goto           .again
    .right:
        const_wide     v2, 0.25
        invoke_virtual "Lapp/hello/ray/Board;->turn(D)V", v0, v2, v3

    .again:
        invoke_virtual "Lapp/hello/ray/Board;->cast()V", v0
        invoke_virtual "Landroid/view/View;->invalidate()V", v0
        return_void
    end method
end macro

macro _interfaces
    _main_ifaces deftypelist "Landroid/view/View$OnClickListener;"
end macro

dex_lists _board, _main, _types, _fields

dex_file _interfaces
