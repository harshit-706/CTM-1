<script lang="ts">
  import { createEventDispatcher } from 'svelte';
  
  export let variant: 'default' | 'destructive' | 'secondary' = 'default';
  export let size: 'sm' | 'md' | 'lg' = 'md';
  export let type: 'button' | 'submit' | 'reset' = 'button';
  export let disabled = false;

  const dispatch = createEventDispatcher();

  const baseStyles = 'inline-flex items-center justify-center rounded-md text-sm font-bold transition-all duration-200 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:opacity-50 disabled:pointer-events-none ring-offset-background active:scale-95';
  
  const variants = {
    default: 'bg-primary text-primary-foreground hover:brightness-110 shadow-[0_4px_14px_0_rgba(0,0,0,0.25)] hover:shadow-[0_6px_20px_rgba(0,0,0,0.23)]',
    destructive: 'bg-destructive text-destructive-foreground hover:brightness-110 shadow-[0_4px_14px_0_rgba(255,0,0,0.39)] hover:shadow-[0_6px_20px_rgba(255,0,0,0.23)]',
    secondary: 'bg-secondary text-secondary-foreground hover:brightness-110 shadow-[0_4px_14px_0_rgba(0,255,0,0.39)] hover:shadow-[0_6px_20px_rgba(0,255,0,0.23)]'
  };

  const sizes = {
    sm: 'h-9 px-3',
    md: 'h-10 py-2 px-4',
    lg: 'h-11 px-8'
  };
</script>

<button
  {type}
  class="{baseStyles} {variants[variant]} {sizes[size]} {$$props.class || ''}"
  {disabled}
  on:click={(e) => dispatch('click', e)}
>
  <slot />
</button>
