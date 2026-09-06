add_executable(SRB2DSI)
target_link_libraries(SRB2DSI dswifi9 mm9 png z)

set (SRB2_NDS_SOURCES
    i_main.c
    i_video.c
    i_system.c
    i_sound.c
    i_net.c
)

set (SRB2_MAIN_SOURCES
    string.c
    qs22k.c
    d_main.c
    d_clisrv.c
    d_net.c
    d_netfil.c
    d_netcmd.c
    dehacked.c
    z_zone.c
    f_finale.c
    f_wipe.c
    g_game.c
    g_input.c
    am_map.c
    command.c
    console.c
    hu_stuff.c
    i_time.c
    y_inter.c
    st_stuff.c
    m_aatree.c
    m_anigif.c
    m_argv.c
    m_bbox.c
    m_cheat.c
    m_cond.c
    m_easing.c
    m_fixed.c
    m_menu.c
    m_misc.c
    m_perfstats.c
    m_random.c
    m_queue.c
    info.c
    p_ceilng.c
    p_enemy.c
    p_floor.c
    p_inter.c
    p_lights.c
    p_map.c
    p_maputl.c
    p_mobj.c
    p_polyobj.c
    p_saveg.c
    p_setup.c
    p_sight.c
    p_spec.c
    p_telept.c
    p_tick.c
    p_user.c
    p_slopes.c
    tables.c
    r_bsp.c
    r_data.c
    r_draw.c
    r_main.c
    r_fps.c
    r_plane.c
    r_segs.c
    r_sky.c
    r_splats.c
    r_things.c
    screen.c
    v_video.c
    s_sound.c
    sounds.c
    w_wad.c
    filesrch.c
    mserv.c
    i_tcp.c
    lzf.c
    b_bot.c
    lua_script.c
    lua_baselib.c
    lua_mathlib.c
    lua_hooklib.c
    lua_consolelib.c
    lua_infolib.c
    lua_mobjlib.c
    lua_playerlib.c
    lua_skinlib.c
    lua_thinkerlib.c
    lua_maplib.c
    lua_polyobjlib.c
    lua_blockmaplib.c
    lua_hudlib.c
    lua_hudlib_drawlist.c
    lua_colorlib.c

    comptime.c
    md5.c
    config.h.in
)

set (SRB2_BLUA_SOURCES
    lapi.c
    lbaselib.c
    ldo.c
    lfunc.c
    linit.c
    liolib.c
    llex.c
    lmem.c
    lobject.c
    lstate.c
    lstrlib.c
    ltablib.c
    lundump.c
    lzio.c
    lauxlib.c
    lcode.c
    ldebug.c
    ldump.c
    lgc.c
    lopcodes.c
    lparser.c
    lstring.c
    ltable.c
    ltm.c
    lvm.c
    loslib.c
)

list(TRANSFORM SRB2_MAIN_SOURCES PREPEND "${PROJECT_SOURCE_DIR}/src/")
list(TRANSFORM SRB2_BLUA_SOURCES PREPEND "${PROJECT_SOURCE_DIR}/src/blua/")
target_sources(SRB2DSI PRIVATE ${SRB2_NDS_SOURCES} ${SRB2_MAIN_SOURCES} ${SRB2_BLUA_SOURCES})

target_compile_definitions(SRB2DSI PRIVATE NOIPX ZDEBUG NOMD5 CMAKECONFIG NOPOSTPROCESSING HAVE_ZLIB HAVE_PNG)

nds_create_rom(SRB2DSI
    NAME "SRB2 Legacy"
    SUBTITLE "placeholder"
    AUTHOR "Sonic Team Jr."
    ICON "icon.gif"
    NITROFS "${PROJECT_SOURCE_DIR}/assets/installer/"
)
