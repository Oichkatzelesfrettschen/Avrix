import check_docs
from pathlib import Path


def test_main_missing_index(tmp_path, capsys):
    idx = tmp_path / "missing.rst"
    ret = check_docs.main(["--index-file", str(idx)])
    captured = capsys.readouterr()
    assert ret == 2
    assert "Index file not found" in captured.err
