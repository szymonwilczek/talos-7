import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";
import { Info } from "lucide-react";

interface OLEDAlertProps {
  variant: "mixed" | "pure";
}

export function OLEDAlert({ variant }: OLEDAlertProps) {
  const isMixed = variant === "mixed";

  const title = isMixed ? "Unsupported Chars" : "Unicode Mode";
  const description = isMixed
    ? "Shows as '?' on device, but types correctly."
    : "Shows placeholder, but macro works fine.";

  return (
    <div className="relative w-full mt-auto pt-1">
      <Alert className="border-yellow-500/50 bg-black/90 text-yellow-500 p-1.5 h-auto min-h-0 rounded-md flex gap-2 items-start">
        <Info className="h-3 w-3 mt-[1px] shrink-0 text-yellow-500" />
        <div className="flex-1 min-w-0">
          <AlertTitle className="text-[10px] font-bold leading-none tracking-wide text-[#fff000] mb-0.5">
            {title}
          </AlertTitle>
          <AlertDescription className="text-[9px] leading-3 text-gray-300 font-normal block break-words">
            {description}
          </AlertDescription>
        </div>
      </Alert>
    </div>
  );
}
