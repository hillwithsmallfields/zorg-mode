;;;; Convert org files to zorg files

;;; A .zorg file is a bit like a .org file, but the repeated
;;; characters marking a heading have been replaced by a digit
;;; representing the number of them, keywords will be replaced by a
;;; short indication, with a keywords line in the file indicating the
;;; expansions, and the tags likewise.  The keywords line begins with
;;; a vertical bar, and has space-separated keywords, with a vertical
;;; bar (with a space on either side of it) marking the boundary at
;;; which cycling a keyword should go back.  The tags line begins with
;;; a colon, and has a colon before each tag, and none at the end.

(defun org-export-to-zorg (org-file zorg-file)
  "Convert ORG-FILE to ZORG-FILE."
  (interactive "fFile to export from:
FFile to export into: ")
  ;; todo: produce keywords line, shorten keywords
  ;; todo: produce tags line, shorten tags
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
