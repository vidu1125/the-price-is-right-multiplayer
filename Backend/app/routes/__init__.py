import pkgutil
import importlib
from pathlib import Path

IGNORE_MODULES = {
    "db_routes",
}

def register_routes(flask_app):
    package_name = __name__
    package_path = Path(__file__).parent

    for module_info in pkgutil.iter_modules([str(package_path)]):
        module_name = module_info.name

        if module_name.startswith("_") or module_name in IGNORE_MODULES:
            continue

        module = importlib.import_module(f"{package_name}.{module_name}")

        if hasattr(module, "bp"):
            flask_app.register_blueprint(module.bp)
            print(f"✅ Registered blueprint: {module_name}")
