for f in "$@"; do
  echo "===== $f ====="
  cat "$f"
  echo    # línea en blanco para separar
done