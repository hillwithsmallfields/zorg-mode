;;;; Convert org files to zorg files

(defun org-export-to-zorg (org-file zorg-file)
  "Convert ORG-FILE to ZORG-FILE."
  (interactive "fFile to export from: 
FFile to export into: ")
  (save-excursion
    (find-file zorg-file)
    (erase-buffer)
    (insert-file-contents org-file)
    (goto-char (point-min))
    (delete-non-matching-lines "^\\*")
    (goto-char (point-min))
    (while (re-search-forward "^\\(\\*+\\)" (point-max) t)
      (replace-match (int-to-string (- (match-end 1) (match-beginning 1))) nil t))
    (basic-save-buffer))
  )

;;;; org-zorg.el ends here
