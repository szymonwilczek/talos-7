import { motion, AnimatePresence } from 'framer-motion';
import { Progress } from '@/components/ui/progress';
import { Alert, AlertDescription } from '@/components/ui/alert';
import { CheckCircle } from 'lucide-react';

interface SaveFeedbackProps {
  isSaving: boolean;
  saveProgress: number;
  saveSuccess: boolean;
}

export function SaveFeedback({ isSaving, saveProgress, saveSuccess }: SaveFeedbackProps) {
  return (
    <AnimatePresence mode="wait">
      {isSaving && (
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: -20 }}
          transition={{ duration: 0.3 }}
          key="saving"
        >
          <Progress value={saveProgress} />
        </motion.div>
      )}

      {saveSuccess && !isSaving && (
        <motion.div
          initial={{ opacity: 0, scale: 0.95, y: 20 }}
          animate={{ opacity: 1, scale: 1, y: 0 }}
          exit={{ opacity: 0, scale: 0.95, y: -20 }}
          transition={{ duration: 0.4, ease: [0.4, 0, 0.2, 1] }}
          key="success"
        >
          <Alert>
            <CheckCircle className="h-4 w-4" />
            <AlertDescription>Configuration saved successfully!</AlertDescription>
          </Alert>
        </motion.div>
      )}
    </AnimatePresence>
  );
}
