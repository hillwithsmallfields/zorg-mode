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
    (while (not (eobp))
      (cond
       ((looking-at "^\\(\\*+\\)")
	;; turn * sequences into numbers
	;; todo: check they're not too large
	;; todo: convert keys
	;; todo: convert tags
	(replace-match (int-to-string (- (match-end 1) (match-beginning 1))) nil t nil 1))
       ((looking-at "^\\(\\s-+\\)")
	;; start other lines with a single space
	(replace-match " " nil t nil 1))
       (t
	(insert " "))
       )
      (beginning-of-line 2))
    (basic-save-buffer))
  )

;;;; org-zorg.el ends here
