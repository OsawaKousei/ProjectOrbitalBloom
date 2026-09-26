"""Run with Unreal Editor's Python console after building the project module.

Creates a fresh map without replacing the open editor world.
Does not save or discard unrelated dirty assets. Existing study maps are untouched.
Native map creation is used because generic asset duplication retains private BSP
references to the source map in the current MCP toolset.
"""
import unreal


def create_movement_study():
    path = "/Game/MVP/Maps/L_MovementStudy"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("Movement study already exists; leaving it unchanged: " + path)
        return
    mode = unreal.load_class(None, "/Script/ProjectOrbitalBloom.OrbitalBloomMode")
    if mode is None:
        raise RuntimeError("Build and load ProjectOrbitalBloomEditor before creating the map.")
    for name in ("M_Player", "M_Boss", "M_Sky"):
        if not unreal.EditorAssetLibrary.does_asset_exist("/Game/MVP/Art/" + name):
            raise RuntimeError("Required presentation material is missing: " + name)
    factory = unreal.WorldFactory()
    world = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "L_MovementStudy", "/Game/MVP/Maps", unreal.World, factory
    )
    if world is None:
        raise RuntimeError("Could not create movement study world.")
    world.get_world_settings().set_editor_property("default_game_mode", mode)
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, path):
        raise RuntimeError("Could not save movement study map.")
    # An inactive WorldFactory world retains RF_Standalone. Loading the same map
    # immediately can trip Map_Load's world-leak check. Release it after saving.
    package = world.get_outermost()
    world = None
    unloaded, error = unreal.EditorLoadingAndSavingUtils.unload_packages([package])
    if not unloaded:
        unreal.log_warning("Map saved, but restart Editor before opening it: " + str(error))
    unreal.log("Movement study saved: " + path)


create_movement_study()
