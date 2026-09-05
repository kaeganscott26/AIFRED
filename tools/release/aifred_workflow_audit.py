"""Current read-only workflow checks; see docs/DEVELOPMENT.md."""
from repository_audit import main
if __name__ == "__main__":
    raise SystemExit(main("workflow"))
