import { Label } from "@/components/ui/label";
import { Textarea } from "@/components/ui/textarea";

export function MacroFormText({ value, onChange }: { value: string, onChange: (v: string) => void }) {
  return (
    <div className="space-y-2">
      <Label htmlFor="macro-text">Text to Type</Label>
      <Textarea
        id="macro-text"
        value={value}
        onChange={(e) => onChange(e.target.value.slice(0, 32))}
        maxLength={32}
        rows={3}
        placeholder="Enter text to type"
      />
    </div>
  );
}
