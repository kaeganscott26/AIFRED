"""Current read-only admin checks; see docs/DEVELOPMENT.md."""
from repository_audit import main
if __name__ == "__main__":
    raise SystemExit(main("admin"))
