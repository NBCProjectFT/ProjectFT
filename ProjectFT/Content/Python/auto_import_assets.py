"""
Batch import Fab/Sketchfab asset packs into Unreal.

Run from Unreal Editor:
  py "C:/.../ProjectFT/Content/Python/auto_import_assets.py" --source "C:/AssetDrops" --dest "/Game/Assets/Fab/Auto"

Or from command line:
  UnrealEditor.exe ProjectFT.uproject -ExecutePythonScript="C:/.../Content/Python/auto_import_assets.py --source C:/AssetDrops"
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
DEFAULT_SOURCE_DIR = PROJECT_DIR / "ExternalAssets" / "ToImport"
DEFAULT_DEST_ROOT = "/Game/Assets/Fab/Auto"
DEFAULT_TARGET_SIZE_CM = 100.0

MESH_EXTENSIONS = {".fbx", ".glb", ".gltf"}
GLTF_EXTENSIONS = {".glb", ".gltf"}
TEXTURE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".tif", ".tiff", ".bmp", ".exr"}

BASE_COLOR_KEYS = ("basecolor", "base_color", "albedo", "diffuse", "color", "col", "bc")
NORMAL_KEYS = ("normal", "norm", "nrm")
ROUGHNESS_KEYS = ("roughness", "rough", "rgh")
METALLIC_KEYS = ("metallic", "metalness", "metal", "mtl")
AO_KEYS = ("ambientocclusion", "ambient_occlusion", "occlusion", "_ao", " ao")


def log(message: str) -> None:
    unreal.log(f"[AutoImportAssets] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[AutoImportAssets] {message}")


def error(message: str) -> None:
    unreal.log_error(f"[AutoImportAssets] {message}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Import Fab/Sketchfab FBX/GLB packs.")
    parser.add_argument("--source", default=os.environ.get("UE_ASSET_IMPORT_SOURCE", str(DEFAULT_SOURCE_DIR)))
    parser.add_argument("--dest", default=os.environ.get("UE_ASSET_IMPORT_DEST", DEFAULT_DEST_ROOT))
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--no-materials", action="store_true")
    parser.add_argument("--replace", action="store_true")
    parser.add_argument("--skip-textures", action="store_true")
    parser.add_argument("--no-nanite", action="store_true")
    parser.add_argument("--no-scale", action="store_true")
    parser.add_argument("--target-size", type=float, default=DEFAULT_TARGET_SIZE_CM)
    parser.add_argument(
        "--collision",
        type=str.lower,
        choices=("auto", "box", "sphere", "capsule", "convex", "none"),
        default="auto",
    )
    parser.add_argument("--pivot", type=str.lower, choices=("none", "center", "bottom"), default="none")
    return parser.parse_args(sys.argv[1:])


def sanitize_asset_name(name: str) -> str:
    cleaned = re.sub(r"[^0-9A-Za-z_]+", "_", name.strip())
    cleaned = re.sub(r"_+", "_", cleaned).strip("_")
    if not cleaned:
        cleaned = "ImportedAsset"
    if cleaned[0].isdigit():
        cleaned = f"Asset_{cleaned}"
    return cleaned


def normalize_for_match(name: str) -> str:
    return re.sub(r"[^0-9a-z]+", "", name.lower())


def destination_for_file(source_file: Path, source_root: Path, dest_root: str) -> str:
    try:
        relative_parent = source_file.parent.relative_to(source_root)
    except ValueError:
        relative_parent = Path(source_file.parent.name)

    parts = [sanitize_asset_name(part) for part in relative_parent.parts if part not in ("", ".")]

    if source_file.suffix.lower() in GLTF_EXTENSIONS:
        stem = sanitize_asset_name(source_file.stem)
        if parts and parts[-1].lower() == stem.lower():
            parts = parts[:-1]
        return "/".join([dest_root.rstrip("/")] + parts) if parts else dest_root.rstrip("/")

    if not parts:
        parts = [sanitize_asset_name(source_file.stem)]
    return "/".join([dest_root.rstrip("/")] + parts)


def prefix_for_file(source_file: Path) -> str:
    extension = source_file.suffix.lower()
    if extension in MESH_EXTENSIONS:
        return "SM"
    if extension in TEXTURE_EXTENSIONS:
        return "T"
    return "A"


def destination_name_for_file(source_file: Path) -> str:
    stem = sanitize_asset_name(source_file.stem)
    prefix = prefix_for_file(source_file)
    return stem if stem.lower().startswith(f"{prefix.lower()}_") else f"{prefix}_{stem}"


def make_import_task(source_file: Path, destination_path: str, replace_existing: bool) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.filename = str(source_file)
    task.destination_path = destination_path
    task.destination_name = destination_name_for_file(source_file)
    task.automated = True
    task.save = True
    task.replace_existing = replace_existing

    if source_file.suffix.lower() == ".fbx":
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_materials = True
        options.import_textures = True
        options.import_as_skeletal = False
        options.static_mesh_import_data.combine_meshes = False
        options.static_mesh_import_data.generate_lightmap_u_vs = True
        options.static_mesh_import_data.auto_generate_collision = True
        task.options = options

    return task


def collect_source_files(source_dir: Path, include_textures: bool) -> list[Path]:
    extensions = set(MESH_EXTENSIONS)
    if include_textures:
        extensions.update(TEXTURE_EXTENSIONS)
    return sorted(
        path
        for path in source_dir.rglob("*")
        if path.is_file() and path.suffix.lower() in extensions
    )


def import_files(files: list[Path], source_dir: Path, dest_root: str, replace_existing: bool) -> list[str]:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported_paths: list[str] = []

    for source_file in files:
        dest = destination_for_file(source_file, source_dir, dest_root)
        task = make_import_task(source_file, dest, replace_existing)
        log(f"Importing {source_file.name} -> {dest}/{task.destination_name}")
        asset_tools.import_asset_tasks([task])
        imported_paths.extend([str(path) for path in task.imported_object_paths])

    return imported_paths


def find_imported_assets(imported_paths: list[str]) -> list[unreal.Object]:
    assets = []
    for asset_path in imported_paths:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset:
            assets.append(asset)
    return assets


def classify_textures(assets: list[unreal.Object]) -> dict[str, unreal.Texture]:
    textures: dict[str, unreal.Texture] = {}

    for asset in assets:
        if not isinstance(asset, unreal.Texture):
            continue

        name = asset.get_name()
        normalized = normalize_for_match(name)

        if any(key.replace("_", "") in normalized for key in BASE_COLOR_KEYS):
            textures.setdefault("base_color", asset)
        elif any(key.replace("_", "") in normalized for key in NORMAL_KEYS):
            textures.setdefault("normal", asset)
        elif any(key.replace("_", "") in normalized for key in ROUGHNESS_KEYS):
            textures.setdefault("roughness", asset)
        elif any(key.replace("_", "") in normalized for key in METALLIC_KEYS):
            textures.setdefault("metallic", asset)
        elif any(key.replace("_", "") in normalized for key in AO_KEYS):
            textures.setdefault("ao", asset)

    return textures


def configure_texture(texture: unreal.Texture) -> None:
    name = texture.get_name().lower()

    try:
        if any(key in name for key in NORMAL_KEYS):
            texture.compression_settings = unreal.TextureCompressionSettings.TC_NORMALMAP
            texture.srgb = False
        elif any(key in name for key in ROUGHNESS_KEYS + METALLIC_KEYS + AO_KEYS):
            texture.srgb = False
    except Exception as exc:
        warn(f"Could not configure texture {texture.get_name()}: {exc}")


def set_nanite_enabled(static_mesh: unreal.StaticMesh, enabled: bool) -> None:
    try:
        subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        settings = subsystem.get_nanite_settings(static_mesh)
        settings.enabled = enabled
        subsystem.set_nanite_settings(static_mesh, settings, apply_changes=True)
    except Exception as exc:
        action = "enable" if enabled else "disable"
        warn(f"Could not {action} Nanite on {static_mesh.get_name()}: {exc}")


def get_mesh_dimensions(static_mesh: unreal.StaticMesh) -> unreal.Vector | None:
    try:
        bounds = static_mesh.get_bounds()
        extent = bounds.box_extent
        return unreal.Vector(extent.x * 2.0, extent.y * 2.0, extent.z * 2.0)
    except Exception as exc:
        warn(f"Could not read bounds for {static_mesh.get_name()}: {exc}")
        return None


def get_build_settings(static_mesh: unreal.StaticMesh):
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if hasattr(subsystem, "get_lod_build_settings"):
        return subsystem, subsystem.get_lod_build_settings(static_mesh, 0)

    library = getattr(unreal, "EditorStaticMeshLibrary", None)
    if library and hasattr(library, "get_lod_build_settings"):
        return library, library.get_lod_build_settings(static_mesh, 0)

    return None, None


def set_build_settings(static_mesh: unreal.StaticMesh, owner, settings) -> bool:
    if owner is None or settings is None:
        return False

    try:
        if hasattr(owner, "set_lod_build_settings"):
            owner.set_lod_build_settings(static_mesh, 0, settings)
            return True
    except Exception as exc:
        warn(f"Could not set build settings on {static_mesh.get_name()}: {exc}")

    return False


def rebuild_static_mesh(static_mesh: unreal.StaticMesh) -> None:
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        if hasattr(subsystem, "build_static_mesh"):
            subsystem.build_static_mesh(static_mesh)
        elif hasattr(static_mesh, "build"):
            static_mesh.build(False)
    except Exception as exc:
        warn(f"Could not rebuild {static_mesh.get_name()}: {exc}")


def normalize_static_mesh_size(static_mesh: unreal.StaticMesh, target_size_cm: float) -> None:
    if target_size_cm <= 0:
        return

    dimensions = get_mesh_dimensions(static_mesh)
    if not dimensions:
        return

    largest_dimension = max(dimensions.x, dimensions.y, dimensions.z)
    if largest_dimension <= 0.01:
        warn(f"Skipping scale normalization for {static_mesh.get_name()}: invalid bounds")
        return

    scale = target_size_cm / largest_dimension
    if abs(scale - 1.0) < 0.001:
        return

    owner, settings = get_build_settings(static_mesh)
    if not settings:
        warn(f"Could not normalize size for {static_mesh.get_name()}: build settings unavailable")
        return

    try:
        current_scale = settings.get_editor_property("build_scale3d")
    except Exception:
        current_scale = unreal.Vector(1.0, 1.0, 1.0)

    new_scale = unreal.Vector(
        current_scale.x * scale,
        current_scale.y * scale,
        current_scale.z * scale,
    )

    try:
        settings.set_editor_property("build_scale3d", new_scale)
    except Exception:
        settings.build_scale3d = new_scale

    if set_build_settings(static_mesh, owner, settings):
        rebuild_static_mesh(static_mesh)
        log(f"Scaled {static_mesh.get_name()} by {scale:.3f} to target largest size {target_size_cm:.1f} cm")


def collision_shape_from_mode(collision_mode: str):
    shape_type = getattr(unreal, "ScriptingCollisionShapeType", None)
    if not shape_type:
        return None

    mapping = {
        "auto": "BOX",
        "box": "BOX",
        "sphere": "SPHERE",
        "capsule": "CAPSULE",
    }
    enum_name = mapping.get(collision_mode)
    return getattr(shape_type, enum_name, None) if enum_name else None


def generate_static_mesh_collision(static_mesh: unreal.StaticMesh, collision_mode: str) -> None:
    if collision_mode == "none":
        return

    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    library = getattr(unreal, "EditorStaticMeshLibrary", None)

    try:
        if hasattr(subsystem, "remove_collisions"):
            subsystem.remove_collisions(static_mesh)
        elif library and hasattr(library, "remove_collisions"):
            library.remove_collisions(static_mesh)
    except Exception as exc:
        warn(f"Could not remove old collisions on {static_mesh.get_name()}: {exc}")

    try:
        if collision_mode == "convex":
            if hasattr(subsystem, "set_convex_decomposition_collisions"):
                subsystem.set_convex_decomposition_collisions(static_mesh, 4, 16, 100000)
                log(f"Generated convex collision for {static_mesh.get_name()}")
                return
            if library and hasattr(library, "set_convex_decomposition_collisions"):
                library.set_convex_decomposition_collisions(static_mesh, 4, 16, 100000)
                log(f"Generated convex collision for {static_mesh.get_name()}")
                return
            warn(f"Convex collision API unavailable for {static_mesh.get_name()}")
            return

        shape = collision_shape_from_mode(collision_mode)
        if not shape:
            warn(f"Simple collision shape API unavailable for {static_mesh.get_name()}")
            return

        if hasattr(subsystem, "add_simple_collisions"):
            subsystem.add_simple_collisions(static_mesh, shape)
        elif library and hasattr(library, "add_simple_collisions"):
            library.add_simple_collisions(static_mesh, shape)
        else:
            warn(f"Simple collision API unavailable for {static_mesh.get_name()}")
            return

        log(f"Generated {collision_mode} collision for {static_mesh.get_name()}")
    except Exception as exc:
        warn(f"Could not generate collision for {static_mesh.get_name()}: {exc}")


def correct_static_mesh_pivot(static_mesh: unreal.StaticMesh, pivot_mode: str) -> None:
    if pivot_mode == "none":
        return

    dimensions = get_mesh_dimensions(static_mesh)
    if not dimensions:
        return

    pivot_offset = unreal.Vector(0.0, 0.0, 0.0)
    if pivot_mode == "bottom":
        pivot_offset = unreal.Vector(0.0, 0.0, -dimensions.z * 0.5)

    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    attempted = False

    try:
        if hasattr(subsystem, "set_pivot_offset"):
            subsystem.set_pivot_offset(static_mesh, pivot_offset)
            attempted = True
        elif hasattr(static_mesh, "set_editor_property"):
            static_mesh.set_editor_property("pivot_offset", pivot_offset)
            attempted = True
    except Exception as exc:
        warn(f"Pivot correction failed for {static_mesh.get_name()}: {exc}")
        return

    if attempted:
        log(f"Applied {pivot_mode} pivot correction to {static_mesh.get_name()}")
    else:
        warn(
            f"Pivot correction API unavailable for {static_mesh.get_name()}. "
            "Use Modeling Mode's bake pivot tools for exact asset-pivot baking."
        )


def create_material(folder: str, base_name: str, textures: dict[str, unreal.Texture]) -> unreal.Material | None:
    if not textures:
        return None

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_name = f"M_{sanitize_asset_name(base_name)}"
    material_path = f"{folder}/{material_name}"

    existing = unreal.EditorAssetLibrary.load_asset(material_path)
    if isinstance(existing, unreal.Material):
        return existing

    material = asset_tools.create_asset(material_name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        return None

    material_editing = unreal.MaterialEditingLibrary

    def add_texture_sample(key: str, x: int, y: int, prop: unreal.MaterialProperty, output: str = "RGB") -> None:
        texture = textures.get(key)
        if not texture:
            return
        node = material_editing.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, x, y
        )
        node.texture = texture
        if key == "normal":
            try:
                node.sampler_type = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
            except Exception:
                pass
        material_editing.connect_material_property(node, output, prop)

    add_texture_sample("base_color", -500, -200, unreal.MaterialProperty.MP_BASE_COLOR)
    add_texture_sample("normal", -500, 0, unreal.MaterialProperty.MP_NORMAL)
    add_texture_sample("roughness", -500, 200, unreal.MaterialProperty.MP_ROUGHNESS, "R")
    add_texture_sample("metallic", -500, 400, unreal.MaterialProperty.MP_METALLIC, "R")
    add_texture_sample("ao", -500, 600, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")

    try:
        material_editing.layout_material_expressions(material)
        material_editing.recompile_material(material)
    except Exception as exc:
        warn(f"Material graph created but could not be laid out/recompiled: {exc}")

    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    return material


def process_assets(
    assets: list[unreal.Object],
    make_materials: bool,
    enable_mesh_nanite: bool,
    normalize_size: bool,
    target_size_cm: float,
    collision_mode: str,
    pivot_mode: str,
) -> None:
    static_meshes = [asset for asset in assets if isinstance(asset, unreal.StaticMesh)]
    textures = [asset for asset in assets if isinstance(asset, unreal.Texture)]

    for texture in textures:
        configure_texture(texture)

    for static_mesh in static_meshes:
        set_nanite_enabled(static_mesh, enable_mesh_nanite)

    for static_mesh in static_meshes:
        if normalize_size:
            normalize_static_mesh_size(static_mesh, target_size_cm)
        generate_static_mesh_collision(static_mesh, collision_mode)
        correct_static_mesh_pivot(static_mesh, pivot_mode)

    if make_materials and static_meshes:
        texture_map = classify_textures(assets)
        if texture_map:
            first_mesh = static_meshes[0]
            folder = "/".join(first_mesh.get_path_name().split("/")[:-1])
            base_name = re.sub(r"^SM_", "", first_mesh.get_name())
            material = create_material(folder, base_name, texture_map)
            if material:
                for mesh in static_meshes:
                    try:
                        mesh.set_material(0, material)
                        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
                    except Exception as exc:
                        warn(f"Could not assign material to {mesh.get_name()}: {exc}")

    for asset in assets:
        try:
            unreal.EditorAssetLibrary.save_asset(asset.get_path_name())
        except Exception as exc:
            warn(f"Could not save {asset.get_name()}: {exc}")


def fix_redirectors(dest_root: str) -> None:
    try:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        if hasattr(asset_tools, "fixup_redirectors"):
            asset_tools.fixup_redirectors([dest_root])
        else:
            log(f"Redirector fixup API unavailable in this Unreal build; skipped {dest_root}")
    except Exception as exc:
        warn(f"Could not fix redirectors under {dest_root}: {exc}")


def main() -> None:
    args = parse_args()
    source_dir = Path(args.source).expanduser().resolve()
    dest_root = args.dest.rstrip("/")

    if not source_dir.exists():
        warn(f"Source folder does not exist yet: {source_dir}")
        warn("Create it, unzip Fab/Sketchfab packs there, then run this script again.")
        return

    files = collect_source_files(source_dir, include_textures=not args.skip_textures)
    if not files:
        warn(f"No FBX/GLB/GLTF files found in {source_dir}")
        return

    log(f"Found {len(files)} importable files in {source_dir}")
    if args.dry_run:
        for source_file in files:
            dest = destination_for_file(source_file, source_dir, dest_root)
            log(f"DRY RUN: {source_file} -> {dest}/{destination_name_for_file(source_file)}")
        return

    imported_paths = import_files(files, source_dir, dest_root, replace_existing=args.replace)
    assets = find_imported_assets(imported_paths)
    process_assets(
        assets,
        make_materials=not args.no_materials,
        enable_mesh_nanite=not args.no_nanite,
        normalize_size=not args.no_scale,
        target_size_cm=args.target_size,
        collision_mode=args.collision,
        pivot_mode=args.pivot,
    )
    fix_redirectors(dest_root)

    log(f"Done. Imported/processed {len(assets)} assets under {dest_root}")


if __name__ == "__main__":
    main()
